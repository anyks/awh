/**
 * @file: fmk.cpp
 * @date: 2023-03-04
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <sys/fmk.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * decimalPlaces Функция определения количества знаков после запятой
 * @param number число в котором нужно определить количество знаков
 * @return       количество знаков после запятой
 */
static uint8_t decimalPlaces(double number) noexcept {
	// Результат работы функции
	uint8_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Результирующее число
		double intpart = 0;
		// Если у числа нет дробной части
		if(::modf(number, &intpart) > 0){
			// Получаем остаток от деления
			int64_t remainder = -1, item = 0;
			// Если у числа есть дробная часть
			while((::modf(number, &intpart) > 0) && (result < 15)){
				// Если остаток от деления совпадает
				if(((item = (static_cast <int64_t> (intpart) % 10L)) == remainder) && (remainder != 0))
					// Выходим из цикла
					break;
				// Если результат уже собран
				if(result > 0)
					// Запоминаем остаток от деления
					remainder = item;
				// Увеличиваем число на один порядок
				number *= 10.;
				// Считаем количество чисел
				result++;
			}
			// Если собранное число больше нуля
			while(intpart > 0){
				// Если последний символ нулевой
				if((result > 0) && ((static_cast <uint64_t> (intpart) % 10L) == 0)){
					// Уменьшаем размер числа
					result--;
					// Уменьшаем размер числа
					intpart /= 10.;
				// Выходим из цикла
				} else break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Выполняем сброс количества знаков после запятой
		result = 0;
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}

/**
 * Для операционной системы не являющейся OS Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Если используется модуль IDN
	 */
	#if defined(AWH_IDN)
		/**
		 * convertEncoding Функция конвертирования из одной кодировки в другую
		 * @param data данные для конвертирования
		 * @param from название кодировки из которой необходимо выполнить конвертирование
		 * @param to   название кодировки в которую необходимо выпоолнить конвертацию
		 * @return     получившееся в результате значение
		 */
		static string convertEncoding(const string & data, const string & from, const string & to){
			// Результат работы функции
			string result = "";
			// Если данные переданы на вход правильно
			if(!data.empty() && !from.empty() && !to.empty()){
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем инициализацию конвертера
					iconv_t convert = iconv_open(to.c_str(), from.c_str());
					// Если инициализировать конвертер не вышло
					if(convert == (iconv_t)(-1))
						// Выполняем генерацию ошибки
						throw logic_error("Unable to create convertion descriptor");
					// Получаем размер входящей строки
					size_t size = data.size();
					// Выполняем получение указатель на входящую строку
					char * ptr = const_cast <char *> (data.c_str());
					// Выполняем создание буфера для получения результата
					result.resize(6 * data.size(), 0);
					// Выполняем получения указателя на результирующий буфер
					char * output = result.data();
					// Получаем длину результирующего буфера
					size_t length = result.size();
					// Выполняем конвертацию текста из одной кодировки в другую
					const size_t status = iconv(convert, &ptr, &size, &output, &length);
					// Выполняем закрытие конвертера
					iconv_close(convert);
					// Если конвертация не выполнена
					if(status == static_cast <size_t> (-1)){
						// Выполняем очистку полученного результата
						result.clear();
						// Выполняем формирование ответа
						result.append("Unable to convert ");
						result.append(data);
						result.append(" from ");
						result.append(from);
						result.append(" to ");
						result.append(to);
						// Выполняем генерацию ошибки
						throw logic_error(result);
					}
					// Выполняем коррекцию полученной длины строки
					result.resize(result.size() - length);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "%s\n", error.what());
					#endif
				}
			}
			// Выводим результат
			return result;
		}
	#endif
#endif

/**
 * Устанавливаем шаблон функции
 */
template <typename T>
/**
 * split Метод разделения строк на составляющие
 * @param str       строка для поиска
 * @param delim     разделитель
 * @param container контенер содержащий данные
 * @return          контенер содержащий данные
 */
static T & split(const string & str, const string & delim, T & container) noexcept {
	/**
	 * trimFn Метод удаления пробелов вначале и конце текста
	 * @param text текст для удаления пробелов
	 * @return     результат работы функции
	 */
	auto trimFn = [](string & text) noexcept -> string & {
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем удаление пробелов в начале текста
			text.erase(text.begin(), find_if_not(text.begin(), text.end(), [](char c) noexcept -> bool {
				// Выполняем проверку символа на наличие пробела
				return (::isspace(c) || (c == 32) || (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f') || (c == '\v'));
			}));
			// Выполняем удаление пробелов в конце текста
			text.erase(find_if_not(text.rbegin(), text.rend(), [](char c) noexcept -> bool {
				// Выполняем проверку символа на наличие пробела
				return (::isspace(c) || (c == 32) || (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f') || (c == '\v'));
			}).base(), text.end());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
		// Выводим результат
		return text;
	};
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Очищаем словарь
		container.clear();
		// Результат работы функции
		string result = "";
		// Получаем счётчики перебора
		size_t index = 0, pos = str.find(delim);
		// Выполняем разбиение строк
		while(pos != string::npos){
			// Получаем полученный текст
			result = str.substr(index, pos - index);
			// Вставляем полученный результат в контейнер
			container.insert(container.end(), trimFn(result));
			// Выполняем смещение в тексте
			index = ++pos + (delim.length() - 1);
			// Выполняем поиск разделителя в тексте
			pos = str.find(delim, pos);
			// Если мы дошли до конца текста
			if(pos == string::npos){
				// Получаем полученный текст
				result = str.substr(index, str.length());
				// Вставляем полученный результат в контейнер
				container.insert(container.end(), trimFn(result));
			}
		}
		// Если слово передано а вектор пустой, тогда создаем вектори из 1-го элемента
		if(!str.empty() && container.empty()){
			// Получаем полученный текст
			result = str.substr(index, pos - index);
			// Вставляем полученный результат в контейнер
			container.insert(container.end(), trimFn(result));
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return container;
}
/**
 * Устанавливаем шаблон функции
 */
template <typename T>
/**
 * split Метод разделения строк на составляющие
 * @param str       строка для поиска
 * @param delim     разделитель
 * @param container контенер содержащий данные
 * @return          контенер содержащий данные
 */
static T & split(const wstring & str, const wstring & delim, T & container) noexcept {
	/**
	 * trimFn Метод удаления пробелов вначале и конце текста
	 * @param text текст для удаления пробелов
	 * @return     результат работы функции
	 */
	auto trimFn = [](wstring & text) noexcept -> wstring & {
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем удаление пробелов в начале текста
			text.erase(text.begin(), find_if_not(text.begin(), text.end(), [](wchar_t c) noexcept -> bool {
				// Выполняем проверку символа на наличие пробела
				return (::iswspace(c) || (c == 32) || (c == 160) || (c == 173) || (c == L' ') || (c == L'\t') || (c == L'\n') || (c == L'\r') || (c == L'\f') || (c == L'\v'));
			}));
			// Выполняем удаление пробелов в конце текста
			text.erase(find_if_not(text.rbegin(), text.rend(), [](wchar_t c) noexcept -> bool {
				// Выполняем проверку символа на наличие пробела
				return (::iswspace(c) || (c == 32) || (c == 160) || (c == 173) || (c == L' ') || (c == L'\t') || (c == L'\n') || (c == L'\r') || (c == L'\f') || (c == L'\v'));
			}).base(), text.end());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
		// Выводим результат
		return text;
	};
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Очищаем словарь
		container.clear();
		// Результат работы функции
		wstring result = L"";
		// Получаем счётчики перебора
		size_t index = 0, pos = str.find(delim);
		// Выполняем разбиение строк
		while(pos != wstring::npos){
			// Получаем полученный текст
			result = str.substr(index, pos - index);
			// Вставляем полученный результат в контейнер
			container.insert(container.end(), trimFn(result));
			// Выполняем смещение в тексте
			index = ++pos + (delim.length() - 1);
			// Выполняем поиск разделителя в тексте
			pos = str.find(delim, pos);
			// Если мы дошли до конца текста
			if(pos == wstring::npos){
				// Получаем полученный текст
				result = str.substr(index, str.length());
				// Вставляем полученный результат в контейнер
				container.insert(container.end(), trimFn(result));
			}
		}
		// Если слово передано а вектор пустой, тогда создаем вектори из 1-го элемента
		if(!str.empty() && container.empty()){
			// Получаем полученный текст
			result = str.substr(index, pos - index);
			// Вставляем полученный результат в контейнер
			container.insert(container.end(), trimFn(result));
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return container;
}

/**
 * RomanNumerals структура Римских чисел
 */
static struct RomanNumerals {
	// Шаблоны римских форматов
	const wstring m[5]  = {L"", L"M", L"MM", L"MMM", L"MMMM"};
	const wstring i[10] = {L"", L"I", L"II", L"III", L"IV", L"V", L"VI", L"VII", L"VIII", L"IX"};
	const wstring x[10] = {L"", L"X", L"XX", L"XXX", L"XL", L"L", L"LX", L"LXX", L"LXXX", L"XC"};
	const wstring c[10] = {L"", L"C", L"CC", L"CCC", L"CD", L"D", L"DC", L"DCC", L"DCCC", L"CM"};
} romanNumerals;
/**
 * Symbols Класс основных символов
 */
static class Symbols {
	private:
		// Контейнер римских чисел
		map <char, uint16_t> _romes;
		// Контейнер арабских чисел
		map <char, uint8_t> _arabics;
	private:
		// Контейнер римских чисел для UTF-8
		map <wchar_t, uint16_t> _wideRomes;
		// Контейнер арабских чисел для UTF-8
		map <wchar_t, uint8_t> _wideArabics;
	private:
		// Контейнер латинских символов
		map <char, wchar_t> _letters;
		// Контейнер латинских символов для UTF-8
		map <wchar_t, char> _wideLetters;
	public:
		/**
		 * isRome Метод проверки соответствия римской цифре
		 * @param num римская цифра для проверки
		 * @return    результат проверки
		 */
		bool isRome(const char num) const noexcept {
			// Выполняем проверку сущестования цифры
			return (this->_romes.find(::toupper(num)) != this->_romes.end());
		}
		/**
		 * isRome Метод проверки соответствия римской цифре
		 * @param num римская цифра для проверки
		 * @return    результат проверки
		 */
		bool isRome(const wchar_t num) const noexcept {
			// Выполняем проверку сущестования цифры
			return (this->_wideRomes.find(::towupper(num)) != this->_wideRomes.end());
		}
	public:
		/**
		 * isArabic Метод проверки соответствия арабской цифре
		 * @param num арабская цифра для проверки
		 * @return    результат проверки
		 */
		bool isArabic(const char num) const noexcept {
			// Выполняем проверку сущестования цифры
			return (this->_arabics.find(num) != this->_arabics.end());
		}
		/**
		 * isArabic Метод проверки соответствия арабской цифре
		 * @param num арабская цифра для проверки
		 * @return    результат проверки
		 */
		bool isArabic(const wchar_t num) const noexcept {
			// Выполняем проверку сущестования цифры
			return (this->_wideArabics.find(num) != this->_wideArabics.end());
		}
	public:
		/**
		 * isLetter Метод проверки соответствия латинской букве
		 * @param letter латинская буква для проверки
		 * @return       результат проверки
		 */
		bool isLetter(const char letter) const noexcept {
			// Выполняем проверку сущестования латинской буквы
			return (this->_letters.find(::tolower(letter)) != this->_letters.end());
		}
		/**
		 * isLetter Метод проверки соответствия латинской букве
		 * @param letter латинская буква для проверки
		 * @return       результат проверки
		 */
		bool isLetter(const wchar_t letter) const noexcept {
			// Выполняем проверку сущестования латинской буквы
			return (this->_wideLetters.find(::towlower(letter)) != this->_wideLetters.end());
		}
	public:
		/**
		 * getRome Метод извлечения римской цифры
		 * @param num римская цифра для извлечения
		 * @return    арабская цифрва в виде числа
		 */
		uint16_t getRome(const char num) const noexcept {
			// Результат работы функции
			uint16_t result = 0;
			// Выполняем поиск римского числа
			auto i = this->_romes.find(::toupper(num));
			// Если римское число найдено
			if(i != this->_romes.end())
				// Получаем римское число в чистом виде
				result = i->second;
			// Выводим результат
			return result;
		}
		/**
		 * getRome Метод извлечения римской цифры
		 * @param num римская цифра для извлечения
		 * @return    арабская цифрва в виде числа
		 */
		uint16_t getRome(const wchar_t num) const noexcept {
			// Результат работы функции
			uint16_t result = 0;
			// Выполняем поиск римского числа
			auto i = this->_wideRomes.find(::towupper(num));
			// Если римское число найдено
			if(i != this->_wideRomes.end())
				// Получаем римское число в чистом виде
				result = i->second;
			// Выводим результат
			return result;
		}
	public:
		/**
		 * getArabic Метод извлечения арабской цифры
		 * @param num арабская цифра для извлечения
		 * @return    арабская цифрва в виде числа
		 */
		uint8_t getArabic(const char num) const noexcept {
			// Результат работы функции
			uint8_t result = 0;
			// Выполняем поиск арабского числа
			auto i = this->_arabics.find(num);
			// Если арабское число найдено
			if(i != this->_arabics.end())
				// Получаем арабское число в чистом виде
				result = i->second;
			// Выводим результат
			return result;
		}
		/**
		 * getArabic Метод извлечения арабской цифры
		 * @param num арабская цифра для извлечения
		 * @return    арабская цифрва в виде числа
		 */
		uint8_t getArabic(const wchar_t num) const noexcept {
			// Результат работы функции
			uint8_t result = 0;
			// Выполняем поиск арабского числа
			auto i = this->_wideArabics.find(num);
			// Если арабское число найдено
			if(i != this->_wideArabics.end())
				// Получаем арабское число в чистом виде
				result = i->second;
			// Выводим результат
			return result;
		}
	public:
		/**
		 * getLetter Метод извлечения латинской буквы
		 * @param letter латинская буква для извлечения
		 * @return       латинская буква в виде символа
		 */
		wchar_t getLetter(const char letter) const noexcept {
			// Результат работы функции
			wchar_t result = 0;
			// Выполняем поиск латинской буквы
			auto i = this->_letters.find(::tolower(letter));
			// Если латинская буква найдена
			if(i != this->_letters.end())
				// Получаем латинскую букву в чистом виде
				result = i->second;
			// Выводим результат
			return result;
		}
		/**
		 * getLetter Метод извлечения латинской буквы
		 * @param letter латинская буква для извлечения
		 * @return       латинская буква в виде символа
		 */
		char getLetter(const wchar_t letter) const noexcept {
			// Результат работы функции
			char result = 0;
			// Выполняем поиск латинской буквы
			auto i = this->_wideLetters.find(::towlower(letter));
			// Если латинская буква найдена
			if(i != this->_wideLetters.end())
				// Получаем латинскую букву в чистом виде
				result = i->second;
			// Выводим результат
			return result;
		}
	public:
		/**
		 * Symbols Конструктор
		 */
		Symbols() noexcept {
			// Выполняем заполнение арабских чисел
			this->_arabics = {
				{'0', 0}, {'1', 1}, {'2', 2},
				{'3', 3}, {'4', 4}, {'5', 5},
				{'6', 6}, {'7', 7}, {'8', 8},
				{'9', 9}
			};
			// Выполняем заполнение арабских чисел для UTF-8
			this->_wideArabics = {
				{L'0', 0}, {L'1', 1}, {L'2', 2},
				{L'3', 3}, {L'4', 4}, {L'5', 5},
				{L'6', 6}, {L'7', 7}, {L'8', 8},
				{L'9', 9}
			};
			// Выполняем заполнение римских чисел
			this->_romes = {
				{'I', 1}, {'V', 5}, {'X', 10},
				{'L', 50}, {'C', 100}, {'D', 500},
				{'M', 1000}
			};
			// Выполняем заполнение римских чисел для UTF-8
			this->_wideRomes = {
				{L'I', 1}, {L'V', 5}, {L'X', 10},
				{L'L', 50}, {L'C', 100}, {L'D', 500},
				{L'M', 1000}
			};
			// Выполняем заполнение латинских символов
			this->_letters = {
				{'a', L'a'}, {'b', L'b'}, {'c', L'c'},
				{'d', L'd'}, {'e', L'e'}, {'f', L'f'},
				{'g', L'g'}, {'h', L'h'}, {'i', L'i'},
				{'j', L'j'}, {'k', L'k'}, {'l', L'l'},
				{'m', L'm'}, {'n', L'n'}, {'o', L'o'},
				{'p', L'p'}, {'q', L'q'}, {'r', L'r'},
				{'s', L's'}, {'t', L't'}, {'u', L'u'},
				{'v', L'v'}, {'w', L'w'}, {'x', L'x'},
				{'y', L'y'}, {'z', L'z'}
			};
			// Выполняем заполнение латинских символов для UTF-8
			this->_wideLetters = {
				{L'a', 'a'}, {L'b', 'b'}, {L'c', 'c'},
				{L'd', 'd'}, {L'e', 'e'}, {L'f', 'f'},
				{L'g', 'g'}, {L'h', 'h'}, {L'i', 'i'},
				{L'j', 'j'}, {L'k', 'k'}, {L'l', 'l'},
				{L'm', 'm'}, {L'n', 'n'}, {L'o', 'o'},
				{L'p', 'p'}, {L'q', 'q'}, {L'r', 'r'},
				{L's', 's'}, {L't', 't'}, {L'u', 'u'},
				{L'v', 'v'}, {L'w', 'w'}, {L'x', 'x'},
				{L'y', 'y'}, {L'z', 'z'}
			};
		}
} standardSymbols;

/**
 * is Метод проверки текста на соответствие флагу
 * @param text текст для проверки
 * @param flag флаг проверки
 * @return     результат проверки
 */
bool awh::Framework::is(const char letter, const check_t flag) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если буква передана
	if(letter > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем определение флага проверки
			switch(static_cast <uint8_t> (flag)){
				// Если установлен флаг проверки на печатаемый символ
				case static_cast <uint8_t> (check_t::PRINT):
					// Выполняем проверку символа
					result = (isprint(letter) != 0);
				break;
				// Если установлен флаг проверки на верхний регистр
				case static_cast <uint8_t> (check_t::UPPER):
					// Выполняем проверку совпадают ли символы
					result = (static_cast <int32_t> (letter) == ::toupper(letter));
				break;
				// Если установлен флаг проверки на нижний регистр
				case static_cast <uint8_t> (check_t::LOWER):
					// Выполняем проверку совпадают ли символы
					result = (static_cast <int32_t> (letter) == ::tolower(letter));
				break;
				// Если установлен флаг проверки на пробел
				case static_cast <uint8_t> (check_t::SPACE):
					// Выполняем проверку, является ли символ пробелом
					result = (::isspace(letter) || (letter == 32) || (letter == 9));
				break;
				// Если установлен флаг проверки на латинские символы
				case static_cast <uint8_t> (check_t::LATIAN):
					// Если символ принадлежит к латинскому алфавиту
					result = standardSymbols.isLetter(letter);
				break;
				// Если установлен флаг проверки на число
				case static_cast <uint8_t> (check_t::NUMBER):
					// Если символ принадлежит к цифрам
					result = standardSymbols.isArabic(letter);
				break;
				// Если установлен флаг проверки на соответствие кодировки UTF-8
				case static_cast <uint8_t> (check_t::UTF8):
					// Выполняем проверку симаола на соответствие UTF-8
					result = this->is(string(1, letter), flag);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * is Метод проверки текста на соответствие флагу
 * @param text текст для проверки
 * @param flag флаг проверки
 * @return     результат проверки
 */
bool awh::Framework::is(const wchar_t letter, const check_t flag) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если буква передана
	if(letter > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем определение флага проверки
			switch(static_cast <uint8_t> (flag)){
				// Если установлен флаг проверки на печатаемый символ
				case static_cast <uint8_t> (check_t::PRINT):
					// Выполняем проверку символа
					result = (::iswprint(letter) != 0);
				break;
				// Если установлен флаг проверки на верхний регистр
				case static_cast <uint8_t> (check_t::UPPER):
					// Выполняем проверку совпадают ли символы
					result = (static_cast <wint_t> (letter) == ::towupper(letter));
				break;
				// Если установлен флаг проверки на нижний регистр
				case static_cast <uint8_t> (check_t::LOWER):
					// Выполняем проверку совпадают ли символы
					result = (static_cast <wint_t> (letter) == ::towlower(letter));
				break;
				// Если установлен флаг проверки на пробел
				case static_cast <uint8_t> (check_t::SPACE):
					// Выполняем проверку, является ли символ пробелом
					result = (::iswspace(letter) || (letter == 32) || (letter == 160) || (letter == 173) || (letter == 9));
				break;
				// Если установлен флаг проверки на латинские символы
				case static_cast <uint8_t> (check_t::LATIAN):
					// Если символ принадлежит к латинскому алфавиту
					result = standardSymbols.isLetter(letter);
				break;
				// Если установлен флаг проверки на число
				case static_cast <uint8_t> (check_t::NUMBER):
					// Если символ принадлежит к цифрам
					result = standardSymbols.isArabic(letter);
				break;
				// Если установлен флаг проверки на соответствие кодировки UTF-8
				case static_cast <uint8_t> (check_t::UTF8):
					// Выполняем проверку симаола на соответствие UTF-8
					result = this->is(wstring(1, letter), flag);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * is Метод проверки текста на соответствие флагу
 * @param text текст для проверки
 * @param flag флаг проверки
 * @return     результат проверки
 */
bool awh::Framework::is(const string & text, const check_t flag) const noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем удаление пробелов вокруг текста
	this->transform(text, transform_t::TRIM);
	// Если текст передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем определение флага проверки
			switch(static_cast <uint8_t> (flag)){
				// Если установлен флаг роверки на URL адреса
				case static_cast <uint8_t> (check_t::URL): {
					// Выполняем парсинг nwt адреса
					const auto & url = const_cast <fmk_t *> (this)->_nwt.parse(text);
					// Если ссылка найдена
					result = ((url.type != nwt_t::types_t::NONE) && (url.type != nwt_t::types_t::WRONG));
				} break;
				// Если установлен флаг проверки на печатаемый символ
				case static_cast <uint8_t> (check_t::PRINT): {
					// Выполняем перебор всех символов строки
					for(char letter : text){
						// Выполняем проверку символа
						result = (isprint(letter) != 0);
						// Если символ не печатаемый
						if(!result)
							// Выходим из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на верхний регистр
				case static_cast <uint8_t> (check_t::UPPER): {
					// Выполняем перебор всего слова
					for(auto & letter : text){
						// Выполняем проверку совпадают ли символы
						result = (static_cast <int32_t> (letter) == ::toupper(letter));
						// Если символы не совпадают
						if(!result)
							// Выходим из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на нижний регистр
				case static_cast <uint8_t> (check_t::LOWER): {
					// Выполняем перебор всего слова
					for(auto & letter : text){
						// Выполняем проверку совпадают ли символы
						result = (static_cast <int32_t> (letter) == ::tolower(letter));
						// Если символы не совпадают
						if(!result)
							// Выходим из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на пробел
				case static_cast <uint8_t> (check_t::SPACE): {
					// Выполняем поиск пробела в слове
					for(auto & letter : text){
						// Выполняем проверку, является ли символ пробелом
						result = (::isspace(letter) || (letter == 32) || (letter == 9));
						// Если пробел найден
						if(result)
							// Выходим из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на латинские символы
				case static_cast <uint8_t> (check_t::LATIAN): {
					// Если длина слова больше 1-го символа
					if(text.length() > 1){
						/**
						 * checkFn Функция проверки на валидность символа
						 * @param text  текст для проверки
						 * @param index индекс буквы в слове
						 * @return      результат проверки
						 */
						auto checkFn = [this](const string & text, const size_t index) noexcept -> bool {
							// Результат работы функции
							bool result = false;
							// Получаем текущую букву
							const char letter = text.at(index);
							// Если буква не первая и не последняя
							if((index > 0) && (index < (text.length() - 1))){
								// Получаем предыдущую букву
								const char first = text.at(index - 1);
								// Получаем следующую букву
								const char second = text.at(index + 1);
								// Если проверка не пройдена, проверяем на апостроф
								if(!(result = (((letter == '-') && (first != '-') && (second != '-')) || ::isspace(letter)))){
									// Выполняем проверку на апостроф
									result = (
										(letter == '\'') && (((first != '\'') && (second != '\'')) ||
										(standardSymbols.isLetter(first) && standardSymbols.isLetter(second)))
									);
								}
								// Если результат не получен
								if(!result)
									// Выводим проверку как она есть
									result = standardSymbols.isLetter(letter);
							// Выводим проверку как она есть
							} else result = standardSymbols.isLetter(letter);
							// Выводим результат
							return result;
						};
						// Определяем конец текста
						const uint8_t end = ((text.back() == '!') || (text.back() == '?') ? 2 : 1);
						// Переходим по всем буквам слова
						for(size_t i = 0, j = (text.length() - end); j > ((text.length() / 2) - end); i++, j--){
							// Проверяем является ли слово латинским
							result = (i == j ? checkFn(text, i) : checkFn(text, i) && checkFn(text, j));
							// Если слово не соответствует тогда выходим
							if(!result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ принадлежит к латинскому алфавиту
					} else result = standardSymbols.isLetter(text.front());
				} break;
				// Если установлен флаг проверки на соответствие кодировки UTF-8
				case static_cast <uint8_t> (check_t::UTF8): {
					// Символ для сравнения
					uint32_t cp = 0;
					// Номер позиции для сравнения
					uint8_t num = 0;
					// Получаем байты для сравнения
					const u_char * bytes = reinterpret_cast <const u_char *> (text.c_str());
					// Выполняем перебор всех символов
					while((* bytes) != 0x00){
						// Выполняем проверку первой позиции
						if(((* bytes) & 0x80) == 0x00){
							// U+0000 to U+007F
							// Получаем значение первой части байт
							cp = ((* bytes) & 0x7F);
							// Устанавливаем номер позиции
							num = 1;
						// Выполняем проверку второй позиции
						} else if(((* bytes) & 0xE0) == 0xC0) {
							// U+0080 to U+07FF
							// Получаем значение второй части байт
							cp = ((* bytes) & 0x1F);
							// Устанавливаем номер позиции
							num = 2;
						// Выполняем проверку третей позиции
						} else if(((* bytes) & 0xF0) == 0xE0) {
							// U+0800 to U+FFFF
							// Получаем значение третей части байт
							cp = ((* bytes) & 0x0F);
							// Устанавливаем номер позиции
							num = 3;
						// Выполняем проверку четвёртой позиции
						} else if(((* bytes) & 0xF8) == 0xF0) {
							// U+10000 to U+10FFFF
							// Получаем значение четвёртой части байт
							cp = ((* bytes) & 0x07);
							// Устанавливаем номер позиции
							num = 4;
						// Выходим из функции
						} else return false;
						// Увеличиваем смещение байт
						bytes++;
						// Выполняем перебор всех позиций
						for(uint8_t i = 1; i < num; ++i){
							// Если байты в первой позиции нельзя сопоставить
							if(((* bytes) & 0xC0) != 0x80)
								// Выводим результат проверки
								return false;
							// Выполняем смещение в позиции
							cp = (cp << 6) | ((* bytes) & 0x3F);
							// Увеличиваем смещение байт
							bytes++;
						}
						// Выполняем проверку смещения
						if((cp > 0x10FFFF) ||
						  ((cp <= 0x007F) && (num != 1)) ||
						  ((cp >= 0xD800) && (cp <= 0xDFFF)) ||
						  ((cp >= 0x0080) && (cp <= 0x07FF)  && (num != 2)) ||
						  ((cp >= 0x0800) && (cp <= 0xFFFF)  && (num != 3)) ||
						  ((cp >= 0x10000)&& (cp <= 0x1FFFFF) && (num != 4)))
							// Выводим результат проверки
							return false;
					}
					// Выводим результат
					return true;
				}
				// Если установлен флаг проверки на число
				case static_cast <uint8_t> (check_t::NUMBER): {
					// Если длина слова больше 1-го символа
					if(text.length() > 1){
						// Начальная позиция поиска
						const uint8_t pos = ((text.front() == '-') || (text.front() == '+') ? 1 : 0);
						// Переходим по всем буквам слова
						for(size_t i = static_cast <size_t> (pos), j = (text.length() - 1); j > ((text.length() / 2) - 1); i++, j--){
							// Проверяем является ли слово арабским числом
							result = !(
								(i == j) ?
								!standardSymbols.isArabic(text.at(i)) :
								!standardSymbols.isArabic(text.at(i)) ||
								!standardSymbols.isArabic(text.at(j))
							);
							// Если слово не соответствует тогда выходим
							if(!result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ всего один, проверяем его так
					} else result = standardSymbols.isArabic(text.front());
				} break;
				// Если установлен флаг проверки на число с плавающей точкой
				case static_cast <uint8_t> (check_t::DECIMAL): {
					// Если длина слова больше 1-го символа
					if(text.length() > 1){
						// Текущая буква
						char letter = 0;
						// Начальная позиция поиска
						const uint8_t pos = ((text.front() == '-') || (text.front() == '+') ? 1 : 0);
						// Переходим по всем символам слова
						for(size_t i = static_cast <size_t> (pos); i < text.length(); i++){
							// Если позиция не первая
							if(i > static_cast <size_t> (pos)){
								// Получаем текущую букву
								letter = text.at(i);
								// Если плавающая точка найдена
								if((letter == '.') || (letter == ',')){
									// Проверяем правые и левую части
									result = (
										this->is(text.substr(pos, i - pos), check_t::NUMBER) &&
										this->is(text.substr(i + 1), check_t::NUMBER)
									);
									// Выходим из цикла
									break;
								}
							}
						}
					// Если символ всего один, проверяем его так
					} else result = standardSymbols.isArabic(text.front());
				} break;
				// Если установлен флаг проверки наличия латинских символов в строке
				case static_cast <uint8_t> (check_t::PRESENCE_LATIAN): {
					// Если длина слова больше 1-го символа
					if(text.length() > 1){
						// Переходим по всем буквам слова
						for(size_t i = 0, j = (text.length() - 1); j > ((text.length() / 2) - 1); i++, j--){
							// Проверяем является ли слово латинским
							result = (
								(i == j) ?
								standardSymbols.isLetter(text.at(i)) :
								standardSymbols.isLetter(text.at(i)) ||
								standardSymbols.isLetter(text.at(j))
							);
							// Если найдена хотя бы одна латинская буква тогда выходим
							if(result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ всего один, проверяем его так
					} else result = standardSymbols.isLetter(text.front());
				} break;
				// Если установлен флаг проверки на псевдо-число
				case static_cast <uint8_t> (check_t::PSEUDO_NUMBER): {
					// Если не является то проверяем дальше
					if(!(result = this->is(text, check_t::NUMBER))){
						// Проверяем являются ли первая и последняя буква слова, числом
						result = (standardSymbols.isArabic(text.front()) || standardSymbols.isArabic(text.back()));
						// Если оба варианта не сработали
						if(!result && (text.length() > 2)){
							// Переходим по всему списку
							for(size_t i = 1, j = (text.length() - 2); j > ((text.length() / 2) - 1); i++, j--){
								// Проверяем является ли слово арабским числом
								result = (
									(i == j) ?
									standardSymbols.isArabic(text.at(i)) :
									standardSymbols.isArabic(text.at(i)) ||
									standardSymbols.isArabic(text.at(j))
								);
								// Если хоть один символ является числом, выходим
								if(result)
									// Выполняем выход из цикла
									break;
							}
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
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * is Метод проверки текста на соответствие флагу
 * @param text текст для проверки
 * @param flag флаг проверки
 * @return     результат проверки
 */
bool awh::Framework::is(const wstring & text, const check_t flag) const noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем удаление пробелов вокруг текста
	this->transform(text, transform_t::TRIM);
	// Если текст передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем определение флага проверки
			switch(static_cast <uint8_t> (flag)){
				// Если установлен флаг роверки на URL адреса
				case static_cast <uint8_t> (check_t::URL): {
					// Выполняем парсинг nwt адреса
					const auto & url = const_cast <fmk_t *> (this)->_nwt.parse(this->convert(text));
					// Если ссылка найдена
					result = ((url.type != nwt_t::types_t::NONE) && (url.type != nwt_t::types_t::WRONG));
				} break;
				// Если установлен флаг проверки на печатаемый символ
				case static_cast <uint8_t> (check_t::PRINT): {
					// Выполняем перебор всех символов строки
					for(wchar_t letter : text){
						// Выполняем проверку символа
						result = (::iswprint(letter) != 0);
						// Если символ не печатаемый
						if(!result)
							// Выходим из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на верхний регистр
				case static_cast <uint8_t> (check_t::UPPER): {
					// Выполняем перебор всего слова
					for(auto & letter : text){
						// Выполняем проверку совпадают ли символы
						result = (static_cast <wint_t> (letter) == ::towupper(letter));
						// Если символы не совпадают
						if(!result)
							// Выполняем выход из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на нижний регистр
				case static_cast <uint8_t> (check_t::LOWER): {
					// Выполняем перебор всего слова
					for(auto & letter : text){
						// Выполняем проверку совпадают ли символы
						result = (static_cast <wint_t> (letter) == ::towlower(letter));
						// Если символы не совпадают
						if(!result)
							// Выполняем выход из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на пробел
				case static_cast <uint8_t> (check_t::SPACE): {
					// Выполняем поиск пробела в слове
					for(auto & letter : text){
						// Выполняем проверку, является ли символ пробелом
						result = (::iswspace(letter) || (letter == 32) || (letter == 160) || (letter == 173) || (letter == 9));
						// Если пробел найден
						if(result)
							// Выполняем выход из цикла
							break;
					}
				} break;
				// Если установлен флаг проверки на латинские символы
				case static_cast <uint8_t> (check_t::LATIAN): {
					// Если длина слова больше 1-го символа
					if(text.length() > 1){
						/**
						 * checkFn Функция проверки на валидность символа
						 * @param text  текст для проверки
						 * @param index индекс буквы в слове
						 * @return      результат проверки
						 */
						auto checkFn = [this](const wstring & text, const size_t index) noexcept -> bool {
							// Результат работы функции
							bool result = false;
							// Получаем текущую букву
							const char letter = text.at(index);
							// Если буква не первая и не последняя
							if((index > 0) && (index < (text.length() - 1))){
								// Получаем предыдущую букву
								const char first = text.at(index - 1);
								// Получаем следующую букву
								const char second = text.at(index + 1);
								// Если проверка не пройдена, проверяем на апостроф
								if(!(result = (((letter == L'-') && (first != L'-') && (second != L'-')) || ::iswspace(letter)))){
									// Выполняем проверку на апостроф
									result = (
										(letter == L'\'') && (((first != L'\'') && (second != L'\'')) ||
										(standardSymbols.isLetter(first) && standardSymbols.isLetter(second)))
									);
								}
								// Если результат не получен
								if(!result)
									// Выводим проверку как она есть
									result = standardSymbols.isLetter(letter);
							// Выводим проверку как она есть
							} else result = standardSymbols.isLetter(letter);
							// Выводим результат
							return result;
						};
						// Определяем конец текста
						const uint8_t end = ((text.back() == L'!') || (text.back() == L'?') ? 2 : 1);
						// Переходим по всем буквам слова
						for(size_t i = 0, j = (text.length() - end); j > ((text.length() / 2) - end); i++, j--){
							// Проверяем является ли слово латинским
							result = (i == j ? checkFn(text, i) : checkFn(text, i) && checkFn(text, j));
							// Если слово не соответствует тогда выходим
							if(!result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ принадлежит к латинскому алфавиту
					} else result = standardSymbols.isLetter(text.front());
				} break;
				// Если установлен флаг проверки на соответствие кодировки UTF-8
				case static_cast <uint8_t> (check_t::UTF8): {
					// Символ для сравнения
					uint32_t cp = 0;
					// Номер позиции для сравнения
					uint8_t num = 0;
					// Получаем байты для сравнения
					const wchar_t * bytes = reinterpret_cast <const wchar_t *> (text.c_str());
					// Выполняем перебор всех символов
					while((* bytes) != 0x00){
						// Выполняем проверку первой позиции
						if(((* bytes) & 0x80) == 0x00){
							// U+0000 to U+007F
							// Получаем значение первой части байт
							cp = ((* bytes) & 0x7F);
							// Устанавливаем номер позиции
							num = 1;
						// Выполняем проверку второй позиции
						} else if(((* bytes) & 0xE0) == 0xC0) {
							// U+0080 to U+07FF
							// Получаем значение второй части байт
							cp = ((* bytes) & 0x1F);
							// Устанавливаем номер позиции
							num = 2;
						// Выполняем проверку третей позиции
						} else if(((* bytes) & 0xF0) == 0xE0) {
							// U+0800 to U+FFFF
							// Получаем значение третей части байт
							cp = ((* bytes) & 0x0F);
							// Устанавливаем номер позиции
							num = 3;
						// Выполняем проверку четвёртой позиции
						} else if(((* bytes) & 0xF8) == 0xF0) {
							// U+10000 to U+10FFFF
							// Получаем значение четвёртой части байт
							cp = ((* bytes) & 0x07);
							// Устанавливаем номер позиции
							num = 4;
						// Выходим из функции
						} else return false;
						// Увеличиваем смещение байт
						bytes++;
						// Выполняем перебор всех позиций
						for(uint8_t i = 1; i < num; ++i){
							// Если байты в первой позиции нельзя сопоставить
							if(((* bytes) & 0xC0) != 0x80)
								// Выводим результат проверки
								return false;
							// Выполняем смещение в позиции
							cp = (cp << 6) | ((* bytes) & 0x3F);
							// Увеличиваем смещение байт
							bytes++;
						}
						// Выполняем проверку смещения
						if((cp > 0x10FFFF) ||
						  ((cp <= 0x007F) && (num != 1)) ||
						  ((cp >= 0xD800) && (cp <= 0xDFFF)) ||
						  ((cp >= 0x0080) && (cp <= 0x07FF)  && (num != 2)) ||
						  ((cp >= 0x0800) && (cp <= 0xFFFF)  && (num != 3)) ||
						  ((cp >= 0x10000)&& (cp <= 0x1FFFFF) && (num != 4)))
							// Выводим результат проверки
							return false;
					}
					// Выводим результат
					return true;
				}
				// Если установлен флаг проверки на число
				case static_cast <uint8_t> (check_t::NUMBER): {
					// Если длина слова больше 1-го символа
					if(text.length() > 1){
						// Начальная позиция поиска
						const uint8_t pos = ((text.front() == L'-') || (text.front() == L'+') ? 1 : 0);
						// Переходим по всем буквам слова
						for(size_t i = static_cast <size_t> (pos), j = (text.length() - 1); j > ((text.length() / 2) - 1); i++, j--){
							// Проверяем является ли слово арабским числом
							result = !(
								(i == j) ?
								!standardSymbols.isArabic(text.at(i)) :
								!standardSymbols.isArabic(text.at(i)) ||
								!standardSymbols.isArabic(text.at(j))
							);
							// Если слово не соответствует тогда выходим
							if(!result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ всего один, проверяем его так
					} else result = standardSymbols.isArabic(text.front());
				} break;
				// Если установлен флаг проверки на число с плавающей точкой
				case static_cast <uint8_t> (check_t::DECIMAL): {
					// Если длина слова больше 1-го символа
					if(text.length() > 1){
						// Текущая буква
						char letter = 0;
						// Начальная позиция поиска
						const uint8_t pos = ((text.front() == L'-') || (text.front() == L'+') ? 1 : 0);
						// Переходим по всем символам слова
						for(size_t i = static_cast <size_t> (pos); i < text.length(); i++){
							// Если позиция не первая
							if(i > static_cast <size_t> (pos)){
								// Получаем текущую букву
								letter = text.at(i);
								// Если плавающая точка найдена
								if((letter == L'.') || (letter == L',')){
									// Проверяем правые и левую части
									result = (
										this->is(text.substr(pos, i - pos), check_t::NUMBER) &&
										this->is(text.substr(i + 1), check_t::NUMBER)
									);
									// Выходим из цикла
									break;
								}
							}
						}
					// Если символ всего один, проверяем его так
					} else result = standardSymbols.isArabic(text.front());
				} break;
				// Если установлен флаг проверки наличия латинских символов в строке
				case static_cast <uint8_t> (check_t::PRESENCE_LATIAN): {
					// Если длина слова больше 1-го символа
					if(text.length() > 1){
						// Переходим по всем буквам слова
						for(size_t i = 0, j = (text.length() - 1); j > ((text.length() / 2) - 1); i++, j--){
							// Проверяем является ли слово латинским
							result = (
								(i == j) ?
								standardSymbols.isLetter(text.at(i)) :
								standardSymbols.isLetter(text.at(i)) ||
								standardSymbols.isLetter(text.at(j))
							);
							// Если найдена хотя бы одна латинская буква тогда выходим
							if(result)
								// Выполняем выход из цикла
								break;
						}
					// Если символ всего один, проверяем его так
					} else result = standardSymbols.isLetter(text.front());
				} break;
				// Если установлен флаг проверки на псевдо-число
				case static_cast <uint8_t> (check_t::PSEUDO_NUMBER): {
					// Если не является то проверяем дальше
					if(!(result = this->is(text, check_t::NUMBER))){
						// Проверяем являются ли первая и последняя буква слова, числом
						result = (standardSymbols.isArabic(text.front()) || standardSymbols.isArabic(text.back()));
						// Если оба варианта не сработали
						if(!result && (text.length() > 2)){
							// Переходим по всему списку
							for(size_t i = 1, j = (text.length() - 2); j > ((text.length() / 2) - 1); i++, j--){
								// Проверяем является ли слово арабским числом
								result = (
									(i == j) ?
									standardSymbols.isArabic(text.at(i)) :
									standardSymbols.isArabic(text.at(i)) ||
									standardSymbols.isArabic(text.at(j))
								);
								// Если хоть один символ является числом
								if(result)
									// Выполняем выход из цикла
									break;
							}
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
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * compare Метод сравнения двух строк без учёта регистра
 * @param first  первое слово
 * @param second второе слово
 * @return       результат сравнения
 */
bool awh::Framework::compare(const string & first, const string & second) const noexcept {
	// Выполняем перебор обоих строк
	return ((first.size() == second.size()) ? equal(first.begin(), first.end(), second.begin(), second.end(), [](char a, char b) noexcept -> bool {
		// Выполняем сравнение каждого символа
		return (::tolower(a) == ::tolower(b));
    }) : false);
}
/**
 * compare Метод сравнения двух строк без учёта регистра
 * @param first  первое слово
 * @param second второе слово
 * @return       результат сравнения
 */
bool awh::Framework::compare(const wstring & first, const wstring & second) const noexcept {
	// Выполняем перебор обоих строк
	return ((first.size() == second.size()) ? equal(first.begin(), first.end(), second.begin(), second.end(), [](wchar_t a, wchar_t b) noexcept -> bool {
		// Выполняем сравнение каждого символа
		return (::towlower(a) == ::towlower(b));
    }) : false);
}
/**
 * timestamp Метод получения штампа времени в указанных единицах измерения
 * @param buffer буфер бинарных данных для установки штампа времени
 * @param size   размер бинарных данных штампа времени
 * @param type   тип формируемого штампа времени
 * @param text   флаг извлечения данных в текстовом виде
 */
void awh::Framework::timestamp(void * buffer, const size_t size, const chrono_t type, const bool text) const noexcept {
	// Если буфер данных передан правильно
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если данные извлекаются в текстовом виде
			if(text){
				// Определяем единицы измерения штампа времени
				switch(static_cast <uint8_t> (type)){
					// Если единицы измерения штампа времени требуется получить в годы
					case static_cast <uint8_t> (chrono_t::YEAR): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(static_cast <uint64_t> (hours.count() / static_cast <double> (8760)));
					} break;
					// Если единицы измерения штампа времени требуется получить в месяцах
					case static_cast <uint8_t> (chrono_t::MONTH): {
						// Получаем штамп времени в часы
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(static_cast <uint64_t> (seconds.count() / static_cast <double> (2629746)));
					} break;
					// Если единицы измерения штампа времени требуется получить в неделях
					case static_cast <uint8_t> (chrono_t::WEEK): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(static_cast <uint64_t> (hours.count() / static_cast <double> (168)));
					} break;
					// Если единицы измерения штампа времени требуется получить в днях
					case static_cast <uint8_t> (chrono_t::DAY): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(static_cast <uint64_t> (hours.count() / static_cast <double> (24)));
					} break;
					// Если единицы измерения штампа времени требуется получить в часах
					case static_cast <uint8_t> (chrono_t::HOUR): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(hours.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в минутах
					case static_cast <uint8_t> (chrono_t::MINUTES): {
						// Получаем штамп времени в минуты
						chrono::minutes minutes = chrono::duration_cast <chrono::minutes> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(minutes.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в секундах
					case static_cast <uint8_t> (chrono_t::SECONDS): {
						// Получаем штамп времени в секундах
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(seconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в миллисекундах
					case static_cast <uint8_t> (chrono_t::MILLISECONDS): {
						// Получаем штамп времени в миллисекундах
						chrono::milliseconds milliseconds = chrono::duration_cast <chrono::milliseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(milliseconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в микросекундах
					case static_cast <uint8_t> (chrono_t::MICROSECONDS): {
						// Получаем штамп времени в микросекунды
						chrono::microseconds microseconds = chrono::duration_cast <chrono::microseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(microseconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в наносекундах
					case static_cast <uint8_t> (chrono_t::NANOSECONDS): {
						// Получаем штамп времени в наносекундах
						chrono::nanoseconds nanoseconds = chrono::duration_cast <chrono::nanoseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						(* reinterpret_cast <string *> (buffer)) = std::to_string(nanoseconds.count());
					} break;
				}
			// Если данные извлекаются в виде числа
			} else {
				// Результат работы функции
				uint64_t result = 0;
				// Определяем единицы измерения штампа времени
				switch(static_cast <uint8_t> (type)){
					// Если единицы измерения штампа времени требуется получить в годы
					case static_cast <uint8_t> (chrono_t::YEAR): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / static_cast <double> (8760));
					} break;
					// Если единицы измерения штампа времени требуется получить в месяцах
					case static_cast <uint8_t> (chrono_t::MONTH): {
						// Получаем штамп времени в часы
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (seconds.count() / static_cast <double> (2629746));
					} break;
					// Если единицы измерения штампа времени требуется получить в неделях
					case static_cast <uint8_t> (chrono_t::WEEK): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / static_cast <double> (168));
					} break;
					// Если единицы измерения штампа времени требуется получить в днях
					case static_cast <uint8_t> (chrono_t::DAY): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / static_cast <double> (24));
					} break;
					// Если единицы измерения штампа времени требуется получить в часах
					case static_cast <uint8_t> (chrono_t::HOUR): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в минутах
					case static_cast <uint8_t> (chrono_t::MINUTES): {
						// Получаем штамп времени в минуты
						chrono::minutes minutes = chrono::duration_cast <chrono::minutes> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (minutes.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в секундах
					case static_cast <uint8_t> (chrono_t::SECONDS): {
						// Получаем штамп времени в секундах
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (seconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в миллисекундах
					case static_cast <uint8_t> (chrono_t::MILLISECONDS): {
						// Получаем штамп времени в миллисекундах
						chrono::milliseconds milliseconds = chrono::duration_cast <chrono::milliseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (milliseconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в микросекундах
					case static_cast <uint8_t> (chrono_t::MICROSECONDS): {
						// Получаем штамп времени в микросекунды
						chrono::microseconds microseconds = chrono::duration_cast <chrono::microseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (microseconds.count());
					} break;
					// Если единицы измерения штампа времени требуется получить в наносекундах
					case static_cast <uint8_t> (chrono_t::NANOSECONDS): {
						// Получаем штамп времени в наносекундах
						chrono::nanoseconds nanoseconds = chrono::duration_cast <chrono::nanoseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (nanoseconds.count());
					} break;
				}
				// Определяем размер буфера данных
				switch(size){
					// Если размер данных 1 байт
					case 1: {
						// Получаем максимальное число которое содержит буфер
						const uint8_t length = numeric_limits <uint8_t>::max();
						// Если полученный результат помещается в буфер
						if(result <= static_cast <uint64_t> (length))
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &result, size);
						// Если результат не помещается в буфер данных
						else {
							// Получаем размер множителя
							const uint64_t rate = static_cast <uint64_t> (::pow(10, ::floor(::log10(result))) / ::pow(10, ::floor(::log10(length))));
							// Получаем итоговый результат для вывода
							const uint8_t data = static_cast <uint8_t> ((result - (result % rate)) / rate);
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &data, size);
						}
					} break;
					// Если размер данных 2 байта
					case 2: {
						// Получаем максимальное число которое содержит буфер
						const uint16_t length = numeric_limits <uint16_t>::max();
						// Если полученный результат помещается в буфер
						if(result <= static_cast <uint64_t> (length))
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &result, size);
						// Если результат не помещается в буфер данных
						else {
							// Получаем размер множителя
							const uint64_t rate = static_cast <uint64_t> (::pow(10, ::floor(::log10(result))) / ::pow(10, ::floor(::log10(length))));
							// Получаем итоговый результат для вывода
							const uint16_t data = static_cast <uint16_t> ((result - (result % rate)) / rate);
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &data, size);
						}
					} break;
					// Если размер данных 4 байта
					case 4: {
						// Получаем максимальное число которое содержит буфер
						const uint32_t length = numeric_limits <uint32_t>::max();
						// Если полученный результат помещается в буфер
						if(result <= static_cast <uint64_t> (length))
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &result, size);
						// Если результат не помещается в буфер данных
						else {
							// Получаем размер множителя
							const uint64_t rate = static_cast <uint64_t> (::pow(10, ::floor(::log10(result))) / ::pow(10, ::floor(::log10(length))));
							// Получаем итоговый результат для вывода
							const uint32_t data = static_cast <uint32_t> ((result - (result % rate)) / rate);
							// Выполняем копирование результата в буфер данных
							::memcpy(buffer, &data, size);
						}
					} break;
					// Если размер данных 8 байт
					case 8:
						// Выполняем копирование результата в буфер данных
						::memcpy(buffer, &result, size);
					break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
}
/**
 * iconv Метод конвертирования строки кодировки
 * @param text     текст для конвертирования
 * @param codepage кодировка в которую необходимо сконвертировать текст
 * @return         сконвертированный текст в требуемой кодировке
 */
string awh::Framework::iconv(const string & text, const codepage_t codepage) const noexcept {
	// Результат работы функции
	string result = "";
	// Если текст передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Для операционной системы OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Определяем кодировку в которую нам нужно сконвертировать текст
				switch(static_cast <uint8_t> (codepage)){
					// Если требуется выполнить кодировку в автоматическом режиме
					case static_cast <uint8_t> (codepage_t::AUTO): {
						// Если текст передан в кодировке UTF-8
						if(this->is(text, check_t::UTF8))
							// Выполняем перекодирование в CP1251
							return this->iconv(text, codepage_t::UTF8_CP1251);
						// Выполняем перекодирование в UTF-8
						else return this->iconv(text, codepage_t::CP1251_UTF8);
					} break;
					// Если требуется выполнить кодировку в UTF-8
					case static_cast <uint8_t> (codepage_t::CP1251_UTF8): {
						// Выполняем получение размера буфера данных
						int32_t size = MultiByteToWideChar(1251, 0, text.c_str(), static_cast <int32_t> (text.size()), 0, 0);
						// Если размер буфера данных получен
						if(size > 0){
							// Создаём буфер данных
							vector <wchar_t> buffer(static_cast <size_t> (size), 0);
							// Если конвертация в CP1251 выполнена удачно
							if(MultiByteToWideChar(1251, 0, text.c_str(), static_cast <int32_t> (text.size()), buffer.data(), static_cast <int32_t> (buffer.size()))){
								// Получаем размер результирующего буфера данных в кодировке UTF-8
								size = WideCharToMultiByte(CP_UTF8, 0, buffer.data(), static_cast <int32_t> (buffer.size()), 0, 0, 0, 0);
								// Если размер буфера данных получен
								if(size > 0){
									// Выделяем данные для результирующего буфера данных
									result.resize(static_cast <size_t> (size), 0);
									// Если конвертация буфера текстовых данных в UTF-8 не выполнена
									if(!WideCharToMultiByte(CP_UTF8, 0, buffer.data(), static_cast <int32_t> (buffer.size()), result.data(), static_cast <int32_t> (result.size()), 0, 0)){
										// Выполняем удаление результирующего буфера данных
										result.clear();
										// Выполняем удаление выделенной памяти
										string().swap(result);
									}
								}
							}
						}
					} break;
					// Если требуется выполнить кодировку в CP1251
					case static_cast <uint8_t> (codepage_t::UTF8_CP1251): {
						// Выполняем получение размера буфера данных
						int32_t size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast <int32_t> (text.size()), 0, 0);
						// Если размер буфера данных получен
						if(size > 0){
							// Создаём буфер данных
							vector <wchar_t> buffer(static_cast <size_t> (size), 0);
							// Если конвертация в UTF-8 выполнена удачно
							if(MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast <int32_t> (text.size()), buffer.data(), static_cast <int32_t> (buffer.size()))){
								// Получаем размер результирующего буфера данных в кодировке CP1251
								size = WideCharToMultiByte(1251, 0, buffer.data(), static_cast <int32_t> (buffer.size()), 0, 0, 0, 0);
								// Если размер буфера данных получен
								if(size > 0){
									// Выделяем данные для результирующего буфера данных
									result.resize(static_cast <size_t> (size), 0);
									// Если конвертация буфера текстовых данных в CP1251 не выполнена
									if(!WideCharToMultiByte(1251, 0, buffer.data(), static_cast <int32_t> (buffer.size()), result.data(), static_cast <int32_t> (result.size()), 0, 0)){
										// Выполняем удаление результирующего буфера данных
										result.clear();
										// Выполняем удаление выделенной памяти
										string().swap(result);
									}
								}
							}
						}
					} break;
					// Если кодировка не установлена
					default: return text;
				}
			/**
			 * Для операционной системы не являющейся OS Windows
			 */
			#else
				/**
				 * Если используется модуль IDN
				 */
				#if defined(AWH_IDN)
					// Определяем кодировку в которую нам нужно сконвертировать текст
					switch(static_cast <uint8_t> (codepage)){
						// Если требуется выполнить кодировку в автоматическом режиме
						case static_cast <uint8_t> (codepage_t::AUTO): {
							// Если текст передан в кодировке UTF-8
							if(this->is(text, check_t::UTF8))
								// Выполняем перекодирование в CP1251
								return this->iconv(text, codepage_t::UTF8_CP1251);
							// Выполняем перекодирование в UTF-8
							else return this->iconv(text, codepage_t::CP1251_UTF8);
						} break;
						// Если требуется выполнить кодировку в UTF-8
						case static_cast <uint8_t> (codepage_t::CP1251_UTF8):
							// Выполняем конвертирование строки из CP1251 в UTF-8
							return convertEncoding(text, "CP1251", "UTF-8");
						// Если требуется выполнить кодировку в CP1251
						case static_cast <uint8_t> (codepage_t::UTF8_CP1251):
							// Выполняем конвертирование строки из UTF-8 в CP1251
							return convertEncoding(text, "UTF-8", "CP1251");
						// Если кодировка не установлена
						default: return text;
					}
				/**
				 * Выполняем работу для остальных условий
				 */
				#else
					// Выводим текст как он есть
					return text;
				#endif
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * transform Метод трансформации одного символа
 * @param letter символ для трансформации
 * @param flag   флаг трансформации
 * @return       трансформированный символ
 */
char awh::Framework::transform(char letter, const transform_t flag) const noexcept {
	// Определяем алгоритм трансформации
	switch(static_cast <uint8_t> (flag)){
		// Если передан флаг перевода строки в верхний регистр
		case static_cast <uint8_t> (transform_t::UPPER): {
			// Выполняем перевод символа в верхний регистр
			letter = ::toupper(letter);
		} break;
		// Если передан флаг перевода строки в нижний регистр
		case static_cast <uint8_t> (transform_t::LOWER): {
			// Выполняем перевод символа в нижний регистр
			letter = ::tolower(letter);
		} break;
	}
	// Выводим результат
	return letter;
}
/**
 * transform Метод трансформации одного символа
 * @param letter символ для трансформации
 * @param flag   флаг трансформации
 * @return       трансформированный символ
 */
wchar_t awh::Framework::transform(wchar_t letter, const transform_t flag) const noexcept {
	// Определяем алгоритм трансформации
	switch(static_cast <uint8_t> (flag)){
		// Если передан флаг перевода строки в верхний регистр
		case static_cast <uint8_t> (transform_t::UPPER): {
			// Выполняем перевод символа в верхний регистр
			letter = ::towupper(letter);
		} break;
		// Если передан флаг перевода строки в нижний регистр
		case static_cast <uint8_t> (transform_t::LOWER): {
			// Выполняем перевод символа в нижний регистр
			letter = ::towlower(letter);
		} break;
	}
	// Выводим результат
	return letter;
}
/**
 * transform Метод трансформации строки
 * @param text текст для трансформации
 * @param flag флаг трансформации
 * @return     трансформированная строка
 */
string & awh::Framework::transform(string & text, const transform_t flag) const noexcept {
	// Если текст для обработки передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем алгоритм трансформации
			switch(static_cast <uint8_t> (flag)){
				// Если передан флаг удаления пробелов
				case static_cast <uint8_t> (transform_t::TRIM): {
					// Выполняем удаление пробелов в начале текста
					text.erase(text.begin(), find_if_not(text.begin(), text.end(), [](char c) -> bool {
						// Выполняем проверку символа на наличие пробела
						return (::isspace(c) || (c == 32) || (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f') || (c == '\v'));
					}));
					// Выполняем удаление пробелов в конце текста
					text.erase(find_if_not(text.rbegin(), text.rend(), [](char c) -> bool {
						// Выполняем проверку символа на наличие пробела
						return (::isspace(c) || (c == 32) || (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f') || (c == '\v'));
					}).base(), text.end());
				} break;
				// Если передан флаг перевода строки в верхний регистр
				case static_cast <uint8_t> (transform_t::UPPER): {
					// Выполняем приведение к верхнему регистру
					::transform(text.begin(), text.end(), text.begin(), [](char c){
						// Приводим к верхнему регистру каждую букву
						return ::toupper(c);
					});
				} break;
				// Если передан флаг перевода строки в нижний регистр
				case static_cast <uint8_t> (transform_t::LOWER): {
					// Выполняем приведение к нижнему регистру
					::transform(text.begin(), text.end(), text.begin(), [](char c){
						// Приводим к нижнему регистру каждую букву
						return ::tolower(c);
					});
				} break;
				// Если передан флаг умного перевода начальных символов в верхний регистр
				case static_cast <uint8_t> (transform_t::SMART): {
					// Символ с которым ведётся работа в данный момент
					char letter = 0;
					// Флаг детекции символа
					bool mode = true;
					// Переходим по всем буквам слова и формируем новую строку
					for(size_t i = 0; i < text.length(); i++){
						// Получаем символ с которым ведётся работа в данный момент
						letter = text[i];
						// Если флаг перевода в верхний регистр активирован
						if(mode)
							// Переводим символ в верхний режим
							text[i] = ::toupper(letter);
						// Переводим остальные символы в нижний регистр
						else text[i] = ::tolower(letter);
						// Если найден спецсимвол, устанавливаем флаг детекции
						mode = ((letter == '-') || (letter == '_') || ::isspace(letter) || (letter == 32) || (letter == 9));
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
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return text;
}
/**
 * transform Метод трансформации строки
 * @param text текст для трансформации
 * @param flag флаг трансформации
 * @return     трансформированная строка
 */
wstring & awh::Framework::transform(wstring & text, const transform_t flag) const noexcept {
	// Если текст для обработки передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем алгоритм трансформации
			switch(static_cast <uint8_t> (flag)){
				// Если передан флаг удаления пробелов
				case static_cast <uint8_t> (transform_t::TRIM): {
					// Выполняем удаление пробелов в начале текста
					text.erase(text.begin(), find_if_not(text.begin(), text.end(), [](wchar_t c) -> bool {
						// Выполняем проверку символа на наличие пробела
						return (::iswspace(c) || (c == 32) || (c == 160) || (c == 173) || (c == L' ') || (c == L'\t') || (c == L'\n') || (c == L'\r') || (c == L'\f') || (c == L'\v'));
					}));
					// Выполняем удаление пробелов в конце текста
					text.erase(find_if_not(text.rbegin(), text.rend(), [](wchar_t c) -> bool {
						// Выполняем проверку символа на наличие пробела
						return (::iswspace(c) || (c == 32) || (c == 160) || (c == 173) || (c == L' ') || (c == L'\t') || (c == L'\n') || (c == L'\r') || (c == L'\f') || (c == L'\v'));
					}).base(), text.end());
				} break;
				// Если передан флаг перевода строки в верхний регистр
				case static_cast <uint8_t> (transform_t::UPPER): {
					// Выполняем приведение к верхнему регистру
					::transform(text.begin(), text.end(), text.begin(), [](wchar_t c){
						// Приводим к верхнему регистру каждую букву
						return ::towupper(c);
					});
				} break;
				// Если передан флаг перевода строки в нижний регистр
				case static_cast <uint8_t> (transform_t::LOWER): {
					// Выполняем приведение к нижнему регистру
					::transform(text.begin(), text.end(), text.begin(), [](wchar_t c){
						// Приводим к нижнему регистру каждую букву
						return ::towlower(c);
					});
				} break;
				// Если передан флаг умного перевода начальных символов в верхний регистр
				case static_cast <uint8_t> (transform_t::SMART): {
					// Флаг детекции символа
					bool mode = true;
					// Символ с которым ведётся работа в данный момент
					wchar_t letter = 0;
					// Переходим по всем буквам слова и формируем новую строку
					for(size_t i = 0; i < text.length(); i++){
						// Получаем символ с которым ведётся работа в данный момент
						letter = text[i];
						// Если флаг перевода в верхний регистр активирован
						if(mode)
							// Переводим символ в верхний режим
							text[i] = ::towupper(letter);
						// Переводим остальные символы в нижний регистр
						else text[i] = ::towlower(letter);
						// Если найден спецсимвол, устанавливаем флаг детекции
						mode = ((letter == L'-') || (letter == L'_') || ::iswspace(letter) || (letter == 32) || (letter == 160) || (letter == 173) || (letter == 9));
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
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return text;
}
/**
 * transform Метод трансформации строки
 * @param text текст для трансформации
 * @param flag флаг трансформации
 * @return     трансформированная строка
 */
const string & awh::Framework::transform(const string & text, const transform_t flag) const noexcept {
	// Выполняем трансформацию текста
	return this->transform(* const_cast <string *> (&text), flag);
}
/**
 * transform Метод трансформации строки
 * @param text текст для трансформации
 * @param flag флаг трансформации
 * @return     трансформированная строка
 */
const wstring & awh::Framework::transform(const wstring & text, const transform_t flag) const noexcept {
	// Выполняем трансформацию текста
	return this->transform(* const_cast <wstring *> (&text), flag);
}
/**
 * join Метод объединения списка строк в одну строку
 * @param items список строк которые необходимо объединить
 * @param delim разделитель
 * @return      строка полученная после объединения
 */
string awh::Framework::join(const vector <string> & items, const string & delim) const noexcept {
	// Результат работы функции
	string result = "";
	// Если список строк которые необходимо объединить переданы
	if(!items.empty()){
		// Выполняем перебор всего списка строк
		for(auto & item : items){
			// Если результат ещё не сформирован
			if(!result.empty())
				// Выполняем добавление разделителя
				result.append(delim);
			// Выполняем добавление текущей строки
			result.append(item);
		}
	}
	// Выводим результат
	return result;
}
/**
 * join Метод объединения списка строк в одну строку
 * @param items список строк которые необходимо объединить
 * @param delim разделитель
 * @return      строка полученная после объединения
 */
wstring awh::Framework::join(const vector <wstring> & items, const wstring & delim) const noexcept {
	// Результат работы функции
	wstring result = L"";
	// Если список строк которые необходимо объединить переданы
	if(!items.empty()){
		// Выполняем перебор всего списка строк
		for(auto & item : items){
			// Если результат ещё не сформирован
			if(!result.empty())
				// Выполняем добавление разделителя
				result.append(delim);
			// Выполняем добавление текущей строки
			result.append(item);
		}
	}
	// Выводим результат
	return result;
}
/**
 * split Метод разделения строк на токены
 * @param text      строка для парсинга
 * @param delim     разделитель
 * @param container результирующий вектор
 */
vector <string> & awh::Framework::split(const string & text, const string & delim, vector <string> & container) const noexcept {
	// Выполняем сплит текста
	return ::split(text, delim, container);
}
/**
 * split Метод разделения строк на токены
 * @param text      строка для парсинга
 * @param delim     разделитель
 * @param container результирующий вектор
 */
vector <wstring> & awh::Framework::split(const wstring & text, const wstring & delim, vector <wstring> & container) const noexcept {
	// Выполняем сплит текста
	return ::split(text, delim, container);
}
/**
 * convert Метод конвертирования строки utf-8 в строку
 * @param str строка utf-8 для конвертирования
 * @return    обычная строка
 */
string awh::Framework::convert(const wstring & str) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если строка передана
		if(!str.empty()){
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				result = utf_to_utf <char> (str.c_str(), str.c_str() + str.size());
			// Если нужно использовать стандартную библиотеку
			#else
				// Устанавливаем тип для конвертера UTF-8
				using convert_type = codecvt_utf8 <wchar_t, 0x10ffff, little_endian>;
				// Объявляем конвертер
				wstring_convert <convert_type, wchar_t> conv;
				// wstring_convert <codecvt_utf8 <wchar_t>> conv;
				// Выполняем конвертирование в utf-8 строку
				result = conv.to_bytes(str);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const range_error & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * convert Метод конвертирования строки в строку utf-8
 * @param str строка для конвертирования
 * @return    строка в utf-8
 */
wstring awh::Framework::convert(const string & str) const noexcept {
	// Результат работы функции
	wstring result = L"";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если строка передана
		if(!str.empty()){
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				result = utf_to_utf <wchar_t> (str.c_str(), str.c_str() + str.size());
			// Если нужно использовать стандартную библиотеку
			#else
				// Объявляем конвертер
				// wstring_convert <codecvt_utf8 <wchar_t>> conv;
				wstring_convert <codecvt_utf8_utf16 <wchar_t, 0x10ffff, little_endian>> conv;
				// Выполняем конвертирование в utf-8 строку
				result = conv.from_bytes(str);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const range_error & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * size Метод определения точного размера, сколько занимают данные (в байтах) в буфере
 * @param value значение бинарного буфера для проверки
 * @param size  общий размер бинарного буфера
 * @return      фактический размер буфера занимаемый данными
 */
size_t awh::Framework::size(const void * value, const size_t size) const noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если значение бинарного буфера передано верное
	if((value != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Значение байта с которым будем работать
			uint8_t byte = 0;
			// Получаем общее количество байт буфера
			size_t index = size;
			// Выполняем перебор всех байт буфера
			while(index--){
				// Выполняем получениетекущего байта
				byte = reinterpret_cast <const uint8_t *> (value)[index];
				// Если байты нулевые
				if(byte == 0)
					// Увеличиваем значение результата
					result++;
				// Если байты не нулевые, выходим из цикла
				else break;
			}
			// Формируем окончательный результат
			result = (size - result);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * greater Метод проверки больше первое число второго или нет (бинарным методом)
 * @param value1 значение первого числа в бинарном виде
 * @param value2 значение второго числа в бинарном виде
 * @param size   размер бинарного буфера числа
 * @return       результат проверки
 */
bool awh::Framework::greater(const void * value1, const void * value2, const size_t size) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы правильно
	if((value1 != nullptr) && (value2 != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Значений чисел для сравнения
			bitset <8> num1(0), num2(0);
			// Индекс перебора всех бит числа
			size_t count = 0, index = size;
			// Выполняем перебор всех байт буфера
			while(index--){
				// Получаем значение числа в виде первого байта
				num1 = reinterpret_cast <const uint8_t *> (value1)[index];
				// Получаем значение числа в виде второго байта
				num2 = reinterpret_cast <const uint8_t *> (value2)[index];
				// Получаем первоначальное значение индексов
				count = num1.size();
				// Выполняем перебор всей строки
				while(count--){
					// Если первый байт больше второго
					if((result = (num1.test(count) && !num2.test(count))) || (!num1.test(count) && num2.test(count)))
						// Выходим из функции
						return result;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * itoa Метод конвертации чисел в указанную систему счисления
 * @param value бинарный буфер числа для конвертации
 * @param size  размер бинарного буфера
 * @param radix система счисления
 * @return      полученная строка в указанной системе счисления
 */
string awh::Framework::itoa(const void * value, const size_t size, const uint8_t radix) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные переданы
	if((value != nullptr) && (size > 0) && (radix > 1) && (radix < 37)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Устанавливаем числовые обозначения
			const string digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			// Если запись в бинарном виде
			if(radix == 2){
				// Результат с которым будем работать
				bitset <8> byte(0);
				// Выполняем перебор всего буфера данных
				for(size_t i = 0; i < size; i++){
					// Получаем байт
					byte = reinterpret_cast <const uint8_t *> (value)[i];
					// Переходим по всем байтам полученного бита
					for(size_t j = 0; j < byte.size(); j++){
						// Если бит установлен
						if(byte.test(j))
							// Выполняем добавление первого символа символа
							result.insert(result.begin(), digits[1]);
						// Иначе добавлям нулевой символ
						else result.insert(result.begin(), digits[0]);
					}
				}
			// Если это другая система счисления
			} else {
				// Определяем размер данных для конвертации
				switch(size){
					// Если это один байт
					case 1: {
						// Число с которым будем работать
						uint8_t num = 0;
						// Выполняем копирование полученных данных
						::memcpy(&num, value, size);
						// Особый случай: нулю соответствует не пустая строка, а "0"
						if(num == 0)
							// Выполняем добавление нулевого символа
							result.insert(result.begin(), digits[0]);
						// Раскладываем число на цифры (младшими разрядами вперёд)
						while(num != 0){
							// Добавляем идентификатор числа
							result.insert(result.begin(), digits[num % radix]);
							// Выполняем финальное деление
							num /= radix;
						}
					} break;
					// Если это два байта
					case 2: {
						// Число с которым будем работать
						uint16_t num = 0;
						// Выполняем копирование полученных данных
						::memcpy(&num, value, size);
						// Особый случай: нулю соответствует не пустая строка, а "0"
						if(num == 0)
							// Выполняем добавление нулевого символа
							result.insert(result.begin(), digits[0]);
						// Раскладываем число на цифры (младшими разрядами вперёд)
						while(num != 0){
							// Добавляем идентификатор числа
							result.insert(result.begin(), digits[num % static_cast <uint16_t> (radix)]);
							// Выполняем финальное деление
							num /= static_cast <uint16_t> (radix);
						}
					} break;
					// Если это четыре байта
					case 4: {
						// Число с которым будем работать
						uint32_t num = 0;
						// Выполняем копирование полученных данных
						::memcpy(&num, value, size);
						// Особый случай: нулю соответствует не пустая строка, а "0"
						if(num == 0)
							// Выполняем добавление нулевого символа
							result.insert(result.begin(), digits[0]);
						// Раскладываем число на цифры (младшими разрядами вперёд)
						while(num != 0){
							// Добавляем идентификатор числа
							result.insert(result.begin(), digits[num % static_cast <uint32_t> (radix)]);
							// Выполняем финальное деление
							num /= static_cast <uint32_t> (radix);
						}
					} break;
					// Если это восемь байт
					case 8: {
						// Число с которым будем работать
						uint64_t num = 0;
						// Выполняем копирование полученных данных
						::memcpy(&num, value, size);
						// Особый случай: нулю соответствует не пустая строка, а "0"
						if(num == 0)
							// Выполняем добавление нулевого символа
							result.insert(result.begin(), digits[0]);
						// Раскладываем число на цифры (младшими разрядами вперёд)
						while(num != 0){
							// Добавляем идентификатор числа
							result.insert(result.begin(), digits[num % static_cast <uint64_t> (radix)]);
							// Выполняем финальное деление
							num /= static_cast <uint64_t> (radix);
						}
					} break;
					// Для всех остальных размеров
					default: {
						// Сбрасываем полученный результат
						result.clear();
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Binary data buffer cannot be cast to a number");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "%s\n", "Binary data buffer cannot be cast to a number");
						#endif
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Сбрасываем полученный результат
			result.clear();
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * atoi Метод конвертации строковых чисел в десятичную систему счисления
 * @param value  число в бинарном виде для конвертации в 10-ю систему
 * @param radix  система счисления
 * @param buffer бинарный буфер куда следует положить результат
 * @param size   размер бинарного буфера куда следует положить результат
 */
void awh::Framework::atoi(const string & value, const uint8_t radix, void * buffer, const size_t size) const noexcept {
	// Если данные для конвертации переданы
	if(!value.empty() && (radix > 1) && (radix < 37) && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем перевод в верхний регистр
			string number = value;
			// Позиция в строке алфавита
			size_t pos = string::npos;
			// Устанавливаем числовые обозначения
			const string digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			// Если запись в 16-м виде
			if(radix == 16){
				// Если первые два значения числа являются префиксом
				if(number.compare(0, 2, "0x") == 0)
					// Удаляем первые два символа
					number.erase(0, 2);
			}
			// Выполняем перевод число в верхний регистр
			this->transform(number, transform_t::UPPER);
			// Количество перебираемых элементов
			const uint8_t count = static_cast <uint8_t> (number.length());
			// Определяем размер данных для конвертации
			switch(size){
				// Если это один байт
				case 1: {
					// Результат с которым будем работать
					uint8_t result = 0;
					// Выполняем перебор всех чисел
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number.at(i))) != string::npos)
							// Выполняем перевод в 10-ю систему счисления
							result += (pos * ::pow(radix, count - i - 1));
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Если это два байта
				case 2: {
					// Результат с которым будем работать
					uint16_t result = 0;
					// Выполняем перебор всех чисел
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number.at(i))) != string::npos)
							// Выполняем перевод в 10-ю систему счисления
							result += (pos * ::pow(radix, count - i - 1));
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Если это четыре байта
				case 4: {
					// Результат с которым будем работать
					uint32_t result = 0;
					// Выполняем перебор всех чисел
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number.at(i))) != string::npos)
							// Выполняем перевод в 10-ю систему счисления
							result += (pos * ::pow(radix, count - i - 1));
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Если это восемь байт
				case 8: {
					// Результат с которым будем работать
					uint64_t result = 0;
					// Выполняем перебор всех чисел
					for(uint8_t i = 0; i < count; i++){
						// Если символ найден
						if((pos = digits.find(number.at(i))) != string::npos)
							// Выполняем перевод в 10-ю систему счисления
							result += (pos * ::pow(radix, count - i - 1));
						// Иначе выходим из цикла
						else return;
					}
					// Копируем полученный результат
					::memcpy(buffer, &result, size);
				} break;
				// Для всех остальных размеров
				default: {
					// Если запись в бинарном виде
					if(radix == 2){
						// Значение байта для установки
						uint8_t byte = 0;
						// Результат с которым будем работать
						bitset <8> result(0);
						// Получаем первоначальное значение индексов
						size_t i = value.size(), j = 0, offset = 0;
						// Выполняем перебор всей строки
						while(i--){
							// Если бит положительный
							if(value.at(i) == '1')
								// Устанавливаем бит результата
								result.set(j);
							// Если бит отрицательный, снимаем его
							else result.reset(j);
							// Увеличиваем смещение бит
							j++;
							// Если мы заполнили байт целиком
							if((j % 8) == 0){
								// Сбрасываем значение счётчика
								j = 0;
								// Выполняем получение числа
								byte = static_cast <uint8_t> (result.to_ulong());
								// Выполняем добавление байта в буфер
								::memcpy(reinterpret_cast <uint8_t *> (buffer) + (offset / 8), &byte, sizeof(byte));
								// Увеличиваем смещение в буфере
								offset += 8;
								// Сбрасываем результат
								result.reset();
							}
						}
					// Выводим сообщение об ошибке
					} else {
						// Сбрасываем полученный результат
						::memset(buffer, 0, size);
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Only binary number can be converted to binary buffer");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "%s\n", "Only binary number can be converted to binary buffer");
						#endif
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Сбрасываем полученный результат
			::memset(buffer, 0, size);
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
}
/**
 * noexp Метод перевода числа в безэкспоненциальную форму
 * @param number число для перевода
 * @param step   размер шага после запятой
 * @return       число в безэкспоненциальной форме
 */
string awh::Framework::noexp(const double number, const uint8_t step) const noexcept {
	// Результат работы функции
	string result = "";
	// Если размер шага и число переданы
	if((number > 0.) && (step > 0.)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём поток для конвертации числа
			stringstream ss;
			// Временное значение переменной
			double intpart = 0;
			// Выполняем проверку есть ли дробная часть у числа
			if(::modf(number, &intpart) > 0)
				// Записываем число в поток
				ss << fixed << ::setprecision(step) << number;
			// Записываем число как оно есть
			else ss << fixed << ::setprecision(0) << number;
			// Получаем из потока строку
			ss >> result;
			// Если результат получен
			if(!result.empty()){
				// Переходим по всему числу
				for(auto i = result.begin(); i != result.end();){
					// Если это первый символ
					if(i == result.begin() && ((* i) == '-'))
						// Увеличиваем значение итератора
						++i;
					// Проверяем является ли символ числом
					else if(standardSymbols.isArabic(* i) || ((* i) == '.'))
						// Увеличиваем значение итератора
						++i;
					// Иначе удаляем символ
					else i = result.erase(i);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Сбрасываем полученный результат
			result.clear();
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Если результат не получен
	if(result.empty())
		// Сбрасываем полученный результат
		result = "0";
	// Выводим результат
	return result;
}
/**
 * noexp Метод перевода числа в безэкспоненциальную форму
 * @param number  число для перевода
 * @param onlyNum выводить только числа
 * @return        число в безэкспоненциальной форме
 */
string awh::Framework::noexp(const double number, const bool onlyNum) const noexcept {
	// Результат работы функции
	string result = "";
	// Если размер шага и число переданы
	if(number > 0.){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём поток для конвертации числа
			stringstream ss;
			// Получаем количество знаков после запятой
			const uint8_t count = decimalPlaces(number);
			// Записываем число в поток
			ss << fixed << ::setprecision(count) << number;
			// Получаем из потока строку
			ss >> result;
			// Если результат получен
			if((count > 0) && !result.empty()){
				// Флаг завершения перебора
				bool end = false;
				// Если последний символ является нулём
				while(!end && (result.back() == '0') || (end = ((result.back() == '.') || (result.back() == ','))))
					// Удаляем последний символ
					result.pop_back();
			}
			// Если количество цифр после запятой больше нуля
			if((count > 0) && (result.size() > 2)){
				// Устанавливаем значение последнего символа
				char last = '$';
				// Выполняем перебор всех символов
				for(auto i = (result.end() - 2); i != (result.begin() - 1);){
					// Если символ не является последним
					if(i != (result.end() - 2)){
						// Если символы совпадают
						if((* i) == last)
							// Выполняем удаление лишних символов
							result.erase(i);
						// Если символы не совпадат, выходим
						else break;
					}
					// Запоминаем текущее значение символа
					last = (* i);
					// Уменьшаем значение итератора
					i--;
				}
			}
			// Если нужно выводить только числа
			if(onlyNum && !result.empty()){
				// Переходим по всему числу
				for(auto i = result.begin(); i != result.end();){
					// Если это первый символ
					if(i == result.begin() && ((* i) == '-'))
						// Выполняем увеличение значения итератора
						++i;
					// Проверяем является ли символ числом
					else if(standardSymbols.isArabic(* i) || ((* i) == '.'))
						// Выполняем увеличение значения итератора
						++i;
					// Иначе удаляем символ
					else i = result.erase(i);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Сбрасываем полученный результат
			result.clear();
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Если результат не получен
	if(result.empty())
		// Сбрасываем полученный результат
		result = "0";
	// Выводим результат
	return result;
}
/**
 * rate Метод порверки на сколько процентов (A > B) или (A < B)
 * @param a первое число
 * @param b второе число
 * @return  результат расчёта
 */
float awh::Framework::rate(const float a, const float b) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выводим разницу в процентах
		return ((a > b ? ((a - b) / b * 100.f) : ((b - a) / b * 100.f) * -1.f));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
		// Выводим пустой результат
		return .0f;
	}
}
/**
 * floor Метод приведения количества символов после запятой к указанному количества
 * @param x число для приведения
 * @param n количество символов после запятой
 * @return  сформированное число
 */
double awh::Framework::floor(const double x, const uint8_t n) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем получение разрядности числа
		const double mult = ::pow(10, n);
		// Выполняем приведение числа к указанной разрядности
		return (::floor(x * mult) / mult);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
		// Выводим пустой результат
		return .0;
	}
}
/**
 * rome2arabic Метод перевода римских цифр в арабские
 * @param word римское число
 * @return     арабское число
 */
uint16_t awh::Framework::rome2arabic(const string & word) const noexcept {
	// Результат работы функции
	uint16_t result = 0;
	// Если слово передано
	if(!word.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Символ поиска
			char c, o;
			// Вспомогательные переменные
			uint32_t i = 0, v = 0, n = 0;
			// Получаем длину слова
			const size_t length = word.length();
			// Если слово состоит всего из одной буквы
			if((length == 1) && !standardSymbols.isRome(word.front()))
				// Выводим результат
				return result;
			// Если слово длиннее одной буквы
			else {
				// Переходим по всем буквам слова
				for(size_t i = 0, j = (length - 1); j > ((length / 2) - 1); i++, j--){
					// Проверяем является ли слово римским числом
					if(!((i == j) ?
						standardSymbols.isRome(word.at(i)) :
						standardSymbols.isRome(word.at(i)) &&
						standardSymbols.isRome(word.at(j))
					)) return result;
				}
			}
			// Преобразовываем цифру M
			if(::tolower(word.front()) == 'm'){
				for(n = 0; ::tolower(word[i]) == 'm'; n++)
					i++;
				if(n > 4)
					return 0;
				v += n * 1000;
			}
			// Запоминаем найденный символ
			o = ::tolower(word[i]);
			// Преобразовываем букву D и C
			if((o == 'd') || (o == 'c')){
				if((c = o) == 'd'){
					i++;
					v += 500;
				}
				// Запоминаем найденный символ
				o = ::tolower(word[i + 1]);
				if((c == 'c') && (o == 'm')){
					i += 2;
					v += 900;
				} else if((c == 'c') && (o == 'd')) {
					i += 2;
					v += 400;
				} else {
					for(n = 0; ::tolower(word[i]) == 'c'; n++)
						i++;
					if(n > 4)
						return 0;
					v += n * 100;
				}
			}
			// Запоминаем найденный символ
			o = ::tolower(word[i]);
			// Преобразовываем букву L и X
			if((o == 'l') || (o == 'x')){
				if((c = o) == 'l'){
					i++;
					v += 50;
				}
				// Запоминаем найденный символ
				o = ::tolower(word[i + 1]);
				if((c == 'x') && (o == 'c')){
					i += 2;
					v += 90;
				} else if((c == 'x') && (o == 'l')) {
					i += 2;
					v += 40;
				} else {
					for(n = 0; ::tolower(word[i]) == 'x'; n++)
						i++;
					if(n > 4)
						return 0;
					v += n * 10;
				}
			}
			// Запоминаем найденный символ
			o = ::tolower(word[i]);
			// Преобразовываем букву V и I
			if((o == 'v') || (o == 'i')){
				if((c = o) == 'v'){
					i++;
					v += 5;
				}
				// Запоминаем найденный символ
				o = ::tolower(word[i + 1]);
				if((c == 'i') && (o == 'x')){
					i += 2;
					v += 9;
				} else if((c == 'i') && (o == 'v')) {
					i += 2;
					v += 4;
				} else {
					for(n = 0; ::tolower(word[i]) == 'i'; n++)
						i++;
					if(n > 4)
						return 0;
					v += n;
				}
			}
			// Формируем реузльтат
			result = (((word.length() == i) && (v >= 1) && (v <= 4999)) ? v : 0);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * rome2arabic Метод перевода римских цифр в арабские
 * @param word римское число
 * @return     арабское число
 */
uint16_t awh::Framework::rome2arabic(const wstring & word) const noexcept {
	// Результат работы функции
	uint16_t result = 0;
	// Если слово передано
	if(!word.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Символ поиска
			wchar_t c, o;
			// Вспомогательные переменные
			uint32_t i = 0, v = 0, n = 0;
			// Получаем длину слова
			const size_t length = word.length();
			// Если слово состоит всего из одной буквы
			if((length == 1) && !standardSymbols.isRome(word.front()))
				// Выводим результат
				return result;
			// Если слово длиннее одной буквы
			else {
				// Переходим по всем буквам слова
				for(size_t i = 0, j = (length - 1); j > ((length / 2) - 1); i++, j--){
					// Проверяем является ли слово римским числом
					if(!((i == j) ?
						standardSymbols.isRome(word.at(i)) :
						standardSymbols.isRome(word.at(i)) &&
						standardSymbols.isRome(word.at(j))
					)) return result;
				}
			}
			// Преобразовываем цифру M
			if(::towlower(word.front()) == L'm'){
				for(n = 0; ::towlower(word[i]) == L'm'; n++)
					i++;
				if(n > 4)
					return 0;
				v += n * 1000;
			}
			// Запоминаем найденный символ
			o = ::towlower(word[i]);
			// Преобразовываем букву D и C
			if((o == L'd') || (o == L'c')){
				if((c = o) == L'd'){
					i++;
					v += 500;
				}
				// Запоминаем найденный символ
				o = ::towlower(word[i + 1]);
				if((c == L'c') && (o == L'm')){
					i += 2;
					v += 900;
				} else if((c == L'c') && (o == L'd')) {
					i += 2;
					v += 400;
				} else {
					for(n = 0; ::towlower(word[i]) == L'c'; n++)
						i++;
					if(n > 4)
						return 0;
					v += n * 100;
				}
			}
			// Запоминаем найденный символ
			o = ::towlower(word[i]);
			// Преобразовываем букву L и X
			if((o == L'l') || (o == L'x')){
				if((c = o) == L'l'){
					i++;
					v += 50;
				}
				// Запоминаем найденный символ
				o = ::towlower(word[i + 1]);
				if((c == L'x') && (o == L'c')){
					i += 2;
					v += 90;
				} else if((c == L'x') && (o == L'l')) {
					i += 2;
					v += 40;
				} else {
					for(n = 0; ::towlower(word[i]) == L'x'; n++)
						i++;
					if(n > 4)
						return 0;
					v += n * 10;
				}
			}
			// Запоминаем найденный символ
			o = ::towlower(word[i]);
			// Преобразовываем букву V и I
			if((o == L'v') || (o == L'i')){
				if((c = o) == L'v'){
					i++;
					v += 5;
				}
				// Запоминаем найденный символ
				o = ::towlower(word[i + 1]);
				if((c == L'i') && (o == L'x')){
					i += 2;
					v += 9;
				} else if((c == L'i') && (o == L'v')) {
					i += 2;
					v += 4;
				} else {
					for(n = 0; ::towlower(word[i]) == L'i'; n++)
						i++;
					if(n > 4)
						return 0;
					v += n;
				}
			}
			// Формируем реузльтат
			result = (((word.length() == i) && (v >= 1) && (v <= 4999)) ? v : 0);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * arabic2rome Метод перевода арабских чисел в римские
 * @param number арабское число от 1 до 4999
 * @return       римское число
 */
wstring awh::Framework::arabic2rome(const uint32_t number) const noexcept {
	// Результат работы функции
	wstring result = L"";
	// Если число передано верное
	if((number >= 1) && (number <= 4999)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Копируем полученное число
			uint32_t n = number;
			// Вычисляем до тысяч
			result.append(romanNumerals.m[static_cast <uint8_t> (::floor(n / 1000.))]);
			// Уменьшаем диапазон
			n %= 1000;
			// Вычисляем до сотен
			result.append(romanNumerals.c[static_cast <uint8_t> (::floor(n / 100.))]);
			// Вычисляем до сотен
			n %= 100;
			// Вычисляем до десятых
			result.append(romanNumerals.x[static_cast <uint8_t> (::floor(n / 10.))]);
			// Вычисляем до сотен
			n %= 10;
			// Формируем окончательный результат
			result.append(romanNumerals.i[n]);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * arabic2rome Метод перевода арабских чисел в римские
 * @param word арабское число от 1 до 4999
 * @return     римское число
 */
string awh::Framework::arabic2rome(const string & word) const noexcept {
	// Результат работы функции
	string result = "";
	// Если слово передано
	if(!word.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Преобразуем слово в число
			const uint32_t number = ::stoi(word);
			// Выполняем расчет
			result = this->convert(this->arabic2rome(number));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * arabic2rome Метод перевода арабских чисел в римские
 * @param word арабское число от 1 до 4999
 * @return     римское число
 */
wstring awh::Framework::arabic2rome(const wstring & word) const noexcept {
	// Результат работы функции
	wstring result = L"";
	// Если слово передано
	if(!word.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Преобразуем слово в число
			const uint32_t number = ::stoi(word);
			// Выполняем расчет
			result = this->arabic2rome(number);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * setCase Метод запоминания регистра слова
 * @param pos   позиция для установки регистра
 * @param start начальное значение регистра в бинарном виде
 * @return      позиция верхнего регистра в бинарном виде
 */
uint64_t awh::Framework::setCase(const uint64_t pos, const uint64_t start) const noexcept {
	// Результат работы функции
	uint64_t result = start;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если позиция передана и длина слова тоже
		result += (1 << pos);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * countLetter Метод подсчёта количества указанной буквы в слове
 * @param word   слово в котором нужно подсчитать букву
 * @param letter букву которую нужно подсчитать
 * @return       результат подсчёта
 */
size_t awh::Framework::countLetter(const wstring & word, const wchar_t letter) const noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если слово и буква переданы
	if(!word.empty() && (letter > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Ищем нашу букву
			size_t pos = 0;
			// Выполняем подсчёт количества указанных букв в слове
			while((pos = word.find(letter, pos)) != wstring::npos){
				// Считаем количество букв
				result++;
				// Увеличиваем позицию
				pos++;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * format Метод реализации функции формирования форматированной строки
 * @param format формат строки вывода
 * @param args   передаваемые аргументы
 * @return       сформированная строка
 */
string awh::Framework::format(const char * format, ...) const noexcept {
	// Результат работы функции
	string result = "";
	// Если формат передан
	if(format != nullptr){
		// Создаем список аргументов
		va_list args;
		// Запускаем инициализацию списка аргументов
		va_start(args, format);
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Размер полученной строки
			size_t length = 0;
			// Создаем буфер данных
			result.resize(1024);
			// Выполняем перебор всех аргументов
			while(true){
				// Создаем список аргументов
				va_list args2;
				// Копируем список аргументов
				va_copy(args2, args);
				// Выполняем запись в буфер данных
				length = ::vsnprintf(result.data(), result.size(), format, args2);
				// Если результат получен
				if((length >= 0) && (length < result.size())){
					// Завершаем список аргументов
					va_end(args);
					// Завершаем список локальных аргументов
					va_end(args2);
					// Если результат не получен
					if(length == 0){
						// Выполняем сброс результата
						result.clear();
						// Выходим из функции
						return result;
					// Выводим результат
					} else return result.assign(result.begin(), result.begin() + length);
				}
				// Размер буфера данных
				size_t size = 0;
				// Если данные не получены, увеличиваем буфер в два раза
				if(length < 0)
					// Увеличиваем размер буфера в два раза
					size = (result.size() * 2);
				// Увеличиваем размер буфера на один байт
				else size = (length + 1);
				// Очищаем буфер данных
				result.clear();
				// Выделяем память для буфера
				result.resize(size);
				// Завершаем список локальных аргументов
				va_end(args2);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
		// Завершаем список аргументов
		va_end(args);
	}
	// Выводим результат
	return result;
}
/**
 * format Метод реализации функции формирования форматированной строки
 * @param  format формат строки вывода
 * @param  items  список аргументов строки
 * @return        сформированная строка
 */
string awh::Framework::format(const string & format, const vector <string> & items) const noexcept {
	// Результат работы функции
	string result = format;
	// Если данные переданы
	if(!format.empty() && !items.empty()){
		/**
		 * replaceFn Функция заменты подстроки в строке
		 * @param str  строка в которой нужно произвести замену
		 * @param from строка которую нужно заменить
		 * @param to   строка на которую нужно заменить
		 */
		auto replaceFn = [](string & str, const string & from, const string & to) noexcept {
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Если строка пустая, выходим
				if(from.empty() || to.empty())
					// Выходим из функции
					return;
				// Позиция подстроки в строке
				size_t pos = 0;
				// Выполняем поиск подстроки в стркое
				while((pos = str.find(from, pos)) != string::npos){
					// Заменяем подстроку в строке
					str.replace(pos, from.length(), to);
					// Увеличиваем позицию для поиска в строке
					pos += to.length();
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим сообщение об ошибке
					::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "%s\n", error.what());
				#endif
			}
		};
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Индекс в массиве
			uint16_t index = 1;
			// Исправляем возврат каретки
			replaceFn(result, "\\r", "\r");
			// Исправляем перенос строки
			replaceFn(result, "\\n", "\n");
			// Исправляем табуляцию
			replaceFn(result, "\\t", "\t");
			// Перебираем весь список аргументов
			for(auto & item : items)
				// Выполняем замену индекса аргумента на указанный аргумент
				replaceFn(result, "$" + std::to_string(index++), item);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * exists Метод проверки существования слова в тексте
 * @param word слово для проверки
 * @param text текст в котором выполнения проверка
 * @return     результат выполнения проверки
 */
bool awh::Framework::exists(const string & word, const string & text) const noexcept {
	// Если данные переданы верные
	if(!word.empty() && !text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Индекс позиции символа в тексте
			size_t index = 0;
			// Выполняем поиск слова в тексте
			(void) find_if_not(text.begin(), text.end(), [&index, &word](char c) noexcept -> bool {
				// Если символы в слове совпадают
				if(::tolower(c) == ::tolower(word.at(index)))
					// Увеличиваем значение индекса
					index++;
				// Если символы не совпадают, выполняем сброс индекса
				else index = 0;
				// Если слово полностью было найдено
				return (index != word.size());
			});
			// Выводим результат проверки
			return (index == word.size());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат проверки по умолчанию
	return false;
}
/**
 * exists Метод проверки существования слова в тексте
 * @param word слово для проверки
 * @param text текст в котором выполнения проверка
 * @return     результат выполнения проверки
 */
bool awh::Framework::exists(const wstring & word, const wstring & text) const noexcept {
	// Если данные переданы верные
	if(!word.empty() && !text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Индекс позиции символа в тексте
			size_t index = 0;
			// Выполняем поиск слова в тексте
			(void) find_if_not(text.begin(), text.end(), [&index, &word](wchar_t c) noexcept -> bool {
				// Если символы в слове совпадают
				if(::towlower(c) == ::towlower(word.at(index)))
					// Увеличиваем значение индекса
					index++;
				// Если символы не совпадают, выполняем сброс индекса
				else index = 0;
				// Если слово полностью было найдено
				return (index != word.size());
			});
			// Выводим результат проверки
			return (index == word.size());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат проверки по умолчанию
	return false;
}
/**
 * replace Метод замены в тексте слово на другое слово
 * @param text текст в котором нужно произвести замену
 * @param word слово для поиска
 * @param alt  слово на которое нужно произвести замену
 * @return     результирующий текст
 */
string & awh::Framework::replace(string & text, const string & word, const string & alt) const noexcept {
	// Если текст передан и искомое слово не равно слову для замены
	if(!text.empty() && !word.empty() && !this->compare(word, alt)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Позиция искомого текста
			size_t pos = 0;
			// Определяем текст на который нужно произвести замену
			const string & alternative = (!alt.empty() ? alt : "");
			// Выполняем поиск всех слов
			while((pos = text.find(word, pos)) != string::npos){
				// Выполняем замену текста
				text.replace(pos, word.length(), alternative);
				// Смещаем позицию на единицу
				pos++;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return text;
}
/**
 * replace Метод замены в тексте слово на другое слово
 * @param text текст в котором нужно произвести замену
 * @param word слово для поиска
 * @param alt  слово на которое нужно произвести замену
 * @return     результирующий текст
 */
wstring & awh::Framework::replace(wstring & text, const wstring & word, const wstring & alt) const noexcept {
	// Если текст передан и искомое слово не равно слову для замены
	if(!text.empty() && !word.empty() && !this->compare(word, alt)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Позиция искомого текста
			size_t pos = 0;
			// Определяем текст на который нужно произвести замену
			const wstring & alternative = (!alt.empty() ? alt : L"");
			// Выполняем поиск всех слов
			while((pos = text.find(word, pos)) != wstring::npos){
				// Выполняем замену текста
				text.replace(pos, word.length(), alternative);
				// Смещаем позицию на единицу
				pos++;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return text;
}
/**
 * replace Метод замены в тексте слово на другое слово
 * @param text текст в котором нужно произвести замену
 * @param word слово для поиска
 * @param alt  слово на которое нужно произвести замену
 * @return     результирующий текст
 */
const string & awh::Framework::replace(const string & text, const string & word, const string & alt) const noexcept {
	// Выполняем замену в тексте слово на другое слово
	return this->replace(* const_cast <string *> (&text), word, alt);
}
/**
 * replace Метод замены в тексте слово на другое слово
 * @param text текст в котором нужно произвести замену
 * @param word слово для поиска
 * @param alt  слово на которое нужно произвести замену
 * @return     результирующий текст
 */
const wstring & awh::Framework::replace(const wstring & text, const wstring & word, const wstring & alt) const noexcept {
	// Выполняем замену в тексте слово на другое слово
	return this->replace(* const_cast <wstring *> (&text), word, alt);
}
/**
 * kv Метод извлечения ключей и значений из текста
 * @param text      текст из которого извлекаются записи
 * @param delim     разделитель записей
 * @param separator разделитель ключа и значения
 * @param escaping  символы экранирования
 * @return          список найденных элементов
 */
unordered_map <string, string> awh::Framework::kv(const string & text, const string & delim, const string & separator, const vector <string> & escaping) const noexcept {
	// Результат работы функции
	unordered_map <string, string> result;
	// Если данные для обработки текста передан
	if(!text.empty() && !delim.empty() && !separator.empty() && !escaping.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Количество экранирования
			uint8_t escapingCount = 0;
			// Позиция экранирования
			size_t escapingPosition = 0;
			// Позиции ключа в тексте
			size_t keyBegin = 0, keyEnd = 0;
			// Позиции значения в тексте
			size_t valueBegin = 0, valueEnd = 0;
			// Выполняем парсинг текста
			while(keyBegin < text.length()){
				// Выполняем поиск разделителя ключа и значения
				keyEnd = text.find(separator, keyBegin);
				// Если разделитель не найден, выходим
				if(keyEnd == string::npos)
					// Выходим из цикла
					break;
				// Выполняем поиск позиции начала значения
				valueBegin = (keyEnd + separator.length());
				// Выполняем поиск экранирования разделителя
				const auto i = find_if(escaping.begin(), escaping.end(), [keyEnd, &separator, &text](const string & esc) noexcept -> bool {
					// Выполняем проверку
					return (
						((keyEnd + esc.length() + separator.length()) < text.length()) &&
						(std::strncmp(&text.data()[keyEnd + separator.length()], esc.data(), esc.length()) == 0)
					);
				});
				// Если экранирование найдено
				if(i != escaping.end()){
					// Сбрасываем количество экранирований
					escapingCount = 0;
					// Получаем начало значения
					valueBegin += i->length();
					// Получаем конец значения
					valueEnd = (keyEnd + delim.length());
					/**
					 * Выполняем поиск конца значения
					 */
					do {
						// Устанавливаем количество экранирований на одно значение
						escapingCount = 1;
						// Определяем конец значения
						valueEnd = text.find(* i, valueEnd + i->length() + delim.length());
						// Получаем позицию поиска экранирования
						escapingPosition = (valueEnd - static_cast <size_t> (escapingCount));
						// Если мы нашли экранирование
						while((escapingPosition > 0) && (escapingPosition < text.size()) && (text.at(escapingPosition) == '\\'))
							// Получаем позицию поиска экранирования
							escapingPosition = (valueEnd - static_cast <size_t> (++escapingCount));
					// Если мы ещё не достигли конца значения
					} while((valueEnd != string::npos) && ((escapingCount % 2) == 0));
					// Если конец значения не найден
					if(valueEnd == string::npos)
						// Устанавливаем конец значения последний символ текста
						valueEnd = (text.length() - 1);
				// Если экранирование не найдено
				} else {
					// Устанавливаем конец позиции значения как начало позиции
					valueEnd = valueBegin;
					/**
					 * Выполняем поиск конца строки
					 */
					do {
						// Выполняем поиск разделителя
						valueEnd = text.find(separator, valueEnd + 1);
					/**
					 * Если мы не дошли до конца или нашли экранирование
					 */
					} while((valueEnd != string::npos) && (text[valueEnd - 1] == '\\'));
					// Если разделитель найден
					if(valueEnd != string::npos)
						// Выполняем поиск конца текущей записи
						valueEnd = text.rfind(delim, valueEnd);
					// Если конца значения записи мы не нашли
					if((valueEnd == string::npos) || (valueEnd < valueBegin))
						// Выполняем поиск следующего элемента относительно текущей позиции
						valueEnd = text.find(delim, valueBegin);
					// Если конца значения записи мы не нашли
					if(valueEnd == string::npos)
						// Устанавливаем конец значения последний символ текста
						valueEnd = text.length();
				}
				// Если мы нашли и ключ и значение записи
				if(valueBegin < valueEnd)
					// Выполняем формирование записи результата
					result.emplace(
						text.substr(keyBegin, keyEnd - keyBegin),
						text.substr(valueBegin, valueEnd - valueBegin)
					);
				// Выполняем поиск следующей записи
				keyBegin = (valueEnd + (i != escaping.end() ? i->length() : 0));
				// Выполняем поиск начало следующего ключа
				while(((keyBegin + delim.length()) < text.length()) &&
				       (std::strncmp(&text.data()[keyBegin], delim.data(), delim.length()) == 0))
					// Выполняем установку начала следующего ключа
					keyBegin += delim.length();
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * kv Метод извлечения ключей и значений из текста
 * @param text      текст из которого извлекаются записи
 * @param delim     разделитель записей
 * @param separator разделитель ключа и значения
 * @param escaping  символы экранирования
 * @return          список найденных элементов
 */
unordered_map <wstring, wstring> awh::Framework::kv(const wstring & text, const wstring & delim, const wstring & separator, const vector <wstring> & escaping) const noexcept {
	// Результат работы функции
	unordered_map <wstring, wstring> result;
	// Если данные для обработки текста передан
	if(!text.empty() && !delim.empty() && !separator.empty() && !escaping.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Количество экранирования
			uint8_t escapingCount = 0;
			// Позиция экранирования
			size_t escapingPosition = 0;
			// Позиции ключа в тексте
			size_t keyBegin = 0, keyEnd = 0;
			// Позиции значения в тексте
			size_t valueBegin = 0, valueEnd = 0;
			// Выполняем парсинг текста
			while(keyBegin < text.length()){
				// Выполняем поиск разделителя ключа и значения
				keyEnd = text.find(separator, keyBegin);
				// Если разделитель не найден, выходим
				if(keyEnd == wstring::npos)
					// Выходим из цикла
					break;
				// Выполняем поиск позиции начала значения
				valueBegin = (keyEnd + separator.length());
				// Выполняем поиск экранирования разделителя
				const auto i = find_if(escaping.begin(), escaping.end(), [keyEnd, &separator, &text](const wstring & esc) noexcept -> bool {
					// Выполняем проверку
					return (
						((keyEnd + esc.length() + separator.length()) < text.length()) &&
						(std::wcsncmp(&text.data()[keyEnd + separator.length()], esc.data(), esc.length()) == 0)
					);
				});
				// Если экранирование найдено
				if(i != escaping.end()){
					// Сбрасываем количество экранирований
					escapingCount = 0;
					// Получаем начало значения
					valueBegin += i->length();
					// Получаем конец значения
					valueEnd = (keyEnd + delim.length());
					/**
					 * Выполняем поиск конца значения
					 */
					do {
						// Устанавливаем количество экранирований на одно значение
						escapingCount = 1;
						// Определяем конец значения
						valueEnd = text.find(* i, valueEnd + i->length() + delim.length());
						// Получаем позицию поиска экранирования
						escapingPosition = (valueEnd - static_cast <size_t> (escapingCount));
						// Если мы нашли экранирование
						while((escapingPosition > 0) && (escapingPosition < text.size()) && (text.at(escapingPosition) == '\\'))
							// Получаем позицию поиска экранирования
							escapingPosition = (valueEnd - static_cast <size_t> (++escapingCount));
					// Если мы ещё не достигли конца значения
					} while((valueEnd != wstring::npos) && ((escapingCount % 2) == 0));
					// Если конец значения не найден
					if(valueEnd == wstring::npos)
						// Устанавливаем конец значения последний символ текста
						valueEnd = (text.length() - 1);
				// Если экранирование не найдено
				} else {
					// Устанавливаем конец позиции значения как начало позиции
					valueEnd = valueBegin;
					/**
					 * Выполняем поиск конца строки
					 */
					do {
						// Выполняем поиск разделителя
						valueEnd = text.find(separator, valueEnd + 1);
					/**
					 * Если мы не дошли до конца или нашли экранирование
					 */
					} while((valueEnd != wstring::npos) && (text[valueEnd - 1] == '\\'));
					// Если разделитель найден
					if(valueEnd != wstring::npos)
						// Выполняем поиск конца текущей записи
						valueEnd = text.rfind(delim, valueEnd);
					// Если конца значения записи мы не нашли
					if((valueEnd == wstring::npos) || (valueEnd < valueBegin))
						// Выполняем поиск следующего элемента относительно текущей позиции
						valueEnd = text.find(delim, valueBegin);
					// Если конца значения записи мы не нашли
					if(valueEnd == wstring::npos)
						// Устанавливаем конец значения последний символ текста
						valueEnd = text.length();
				}
				// Если мы нашли и ключ и значение записи
				if(valueBegin < valueEnd)
					// Выполняем формирование записи результата
					result.emplace(
						text.substr(keyBegin, keyEnd - keyBegin),
						text.substr(valueBegin, valueEnd - valueBegin)
					);
				// Выполняем поиск следующей записи
				keyBegin = (valueEnd + (i != escaping.end() ? i->length() : 0));
				// Выполняем поиск начало следующего ключа
				while(((keyBegin + delim.length()) < text.length()) &&
				       (std::wcsncmp(&text.data()[keyBegin], delim.data(), delim.length()) == 0))
					// Выполняем установку начала следующего ключа
					keyBegin += delim.length();
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * domainZone Метод установки пользовательской зоны
 * @param zone пользовательская зона
 */
void awh::Framework::domainZone(const string & zone) noexcept {
	// Если зона передана, устанавливаем её
	if(!zone.empty())
		// Устанавливаем пользовательскую зону
		this->_nwt.zone(zone);
}
/**
 * domainZones Метод установки списка пользовательских зон
 * @param zones список доменных зон интернета
 */
void awh::Framework::domainZones(const set <string> & zones) noexcept {
	// Устанавливаем список доменных зон
	if(!zones.empty())
		// Устанавливаем список пользовательских зон
		this->_nwt.zones(zones);
}
/**
 * domainZones Метод извлечения списка пользовательских зон интернета
 * @return список доменных зон
 */
const set <string> & awh::Framework::domainZones() const noexcept {
	// Выводим список доменных зон интернета
	return this->_nwt.zones();
}
/**
 * setLocale Метод установки системной локали
 * @param locale локализация приложения
 */
void awh::Framework::setLocale(const string & locale) noexcept {
	// Устанавливаем локаль
	if(!locale.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём новую локаль
			// ::locale loc(locale.c_str());
			// Устанавливапм локализацию приложения
			::setlocale(LC_CTYPE, locale.c_str());
			::setlocale(LC_COLLATE, locale.c_str());
			// Устанавливаем локаль системы
			// this->_locale = ::locale::global(loc);
			this->_locale = ::locale(locale.c_str());
			/**
			 * Для операционной системы OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Параметры устанавливаемого шрифта
				CONSOLE_FONT_INFOEX lpConsoleCurrentFontEx;
				// Формируем параметры шрифта
				lpConsoleCurrentFontEx.nFont        = 1;
				lpConsoleCurrentFontEx.dwFontSize.X = 7;
				lpConsoleCurrentFontEx.dwFontSize.Y = 12;
				lpConsoleCurrentFontEx.FontWeight   = 500;
				lpConsoleCurrentFontEx.FontFamily   = FF_DONTCARE;
				lpConsoleCurrentFontEx.cbSize       = sizeof(CONSOLE_FONT_INFOEX);
				// Выполняем установку шрифта Lucida Console
				lstrcpyW(lpConsoleCurrentFontEx.FaceName, L"Lucida Console");
				// Выполняем установку шрифта консоли
				SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), false, &lpConsoleCurrentFontEx);
				// Устанавливаем кодировку ввода текстовых данных в консоле
				SetConsoleCP(CP_UTF8); // 65001
				// Устанавливаем кодировку вывода текстовых данных из консоли
				SetConsoleOutputCP(CP_UTF8);
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
}
/**
 * urls Метод извлечения координат url адресов в строке
 * @param text текст для извлечения url адресов
 * @return     список координат с url адресами
 */
map <size_t, size_t> awh::Framework::urls(const string & text) const noexcept {
	// Результат работы функции
	map <size_t, size_t> result;
	// Если текст передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Позиция найденного nwt адреса
			size_t pos = 0;
			// Выполням поиск ссылок в тексте
			while(pos < text.length()){
				// Выполняем парсинг nwt адреса
				auto resUri = const_cast <fmk_t *> (this)->_nwt.parse(text.substr(pos));
				// Если ссылка найдена
				if(resUri.type != nwt_t::types_t::NONE){
					// Получаем данные слова
					const string & word = resUri.uri;
					// Если это не предупреждение
					if(resUri.type != nwt_t::types_t::WRONG){
						// Если позиция найдена
						if((pos = text.find(word, pos)) != string::npos){
							// Если в списке результатов найдены пустные значения, очищаем список
							if(result.count(string::npos) > 0)
								// Выполняем очистку результата
								result.clear();
							// Добавляем в список нашу ссылку
							result.insert({pos, pos + word.length()});
						// Если ссылка не найдена в тексте, выходим
						} else break;
					}
					// Сдвигаем значение позиции
					pos += word.length();
				// Если uri адрес больше не найден то выходим
				} else break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * icon Метод получения иконки
 * @param end флаг завершения работы
 * @return    иконка напутствия работы
 */
string awh::Framework::icon(const bool end) const noexcept {
	// Список иконок для начала работы
	const vector <string> iconBegin = {
		"🎲","🎰","🏓","🎱","🥚","⚽️",
		"🏀","🏈","⚾️","🥎","🏐","🪙",
		"🎾","🏑","🧲","🏹","🧱","🏋‍♀️",
		"⛹‍♀️","🤽‍♀️","🥁","🕯","🎳","🎮",
		"🙏","🤪","🙄","😏","😊","☺️",
		"😉","🤔","😋","😤","🤥","🧐",
		"🤓","😇","🙃","🤫","🤭","🙂",
		"🤗","🤩","😌","😎","🤡","🤠",
		"🌟","🧠","👀","👁","🏦","🛸",
		"🎬","❤️","📈","🛒","🛎","🤹‍♀️",
		"☝️","🎈","🧚","🕊","✨","⚡️",
		"🌏", "🔥","🪁","🎻","🎲","🎪",
		"🚦","🇷🇺","📺","🏸","🚀","⏳",
		"⏳","♨️","📉","💤","📊","🏳️"
	};
	// Список иконок для конца работы
	const vector <string> iconEnd = {
		"🍾","🎉","🎊","🎈","🎁","🥳",
		"🤩","😍","🥰","🤝","🙌","👐",
		"👌","✌️","🤟","🐝","🎖","🥇",
		"🥈","🥉","🏅","💳","🧨","🚬",
		"🏆","🎯","💎","🔮","🎗","🏵",
		"💪","👍","🪄","💍","⏰","🧮",
		"👸","🤴","🥷","💖","💘","🛍",
		"💝","🧸","💸","🧟‍♂️","💞","👩‍💻",
		"🎀","👅","💋","🚨","🦾","🦠",
		"💩","👾","👼","💥","💫","🌞",
		"🍫","🎂","💯","📰","❤️‍🔥","🎣",
		"🏁","🧾","💶","💷","💴","💵"
	};
	// рандомизация генератора случайных чисел
	::srand(this->timestamp <uint64_t> (chrono_t::NANOSECONDS));
	// Получаем иконку
	return (!end ? iconBegin[::rand() % iconBegin.size()] : iconEnd[::rand() % iconEnd.size()]);
}
/**
 * bytes Метод получения размера в байтах из строки
 * @param str строка обозначения размерности (b, Kb, Mb, Gb, Tb)
 * @return    размер в байтах
 */
double awh::Framework::bytes(const string & str) const noexcept {
	// Размер количество байт
	double result = 0.;
	// Выполняем проверку входящей строки
	const auto & match = this->_regexp.exec(str, this->_bytes);
	// Если данные найдены
	if(!match.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Размерность скорости
			double dimension = 1.;
			// Получаем значение размерности
			result = ::stod(match[1]);
			// Если это размерность в килобайтах
			if(this->compare("Kb", match[2]))
				// Выполняем установку множителя
				dimension = 1024.;
			// Если это размерность в мегабайтах
			else if(this->compare("Mb", match[2]))
				// Выполняем установку множителя
				dimension = 1048576.;
			// Если это размерность в гигабайтах
			else if(this->compare("Gb", match[2]))
				// Выполняем установку множителя
				dimension = 1073741824.;
			// Если это размерность в терабайтах
			else if(this->compare("Tb", match[2]))
				// Выполняем установку множителя
				dimension = 1099511627776.;
			// Если это байты
			else if(this->compare("b", match[2]) || this->compare("bytes", match[2]))
				// Выполняем установку множителя
				dimension = 1.;
			// Если размерность установлена тогда расчитываем количество байт
			if(result > -1.)
				// Устанавливаем размер полученных данных
				result *= dimension;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * bytes Метод конвертации байт в строку
 * @param value   количество байт
 * @param onlyNum выводить только числа
 * @return        полученная строка
 */
string awh::Framework::bytes(const double value, const bool onlyNum) const noexcept {
	// Результат работы функции
	string result = "0 bytes";
	// Если количество байт передано
	if(value > 0.){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Шаблон киллобайта
			const double kb = 1024.;
			// Шаблон мегабайта
			const double mb = 1048576.;
			// Шаблон гигабайта
			const double gb = 1073741824.;
			// Шаблон терабайта
			const double tb = 1099511627776.;
			// Если переданное значение соответствует терабайту
			if(value >= tb){
				// Выполняем копирование терабайта
				result = this->noexp(value / tb, onlyNum);
				// Добавляем наименование единицы измерения
				result.append(" Tb");
			// Если переданное значение соответствует гигабайту
			} else if((value >= gb) && (value < tb)) {
				// Выполняем копирование гигабайта
				result = this->noexp(value / gb, onlyNum);
				// Добавляем наименование единицы измерения
				result.append(" Gb");
			// Если переданное значение соответствует мегабайту
			} else if((value >= mb) && (value < gb)) {
				// Выполняем копирование мегабайта
				result = this->noexp(value / mb, onlyNum);
				// Добавляем наименование единицы измерения
				result.append(" Mb");
			// Если переданное значение соответствует киллобайту
			} else if((value >= kb) && (value < mb)) {
				// Выполняем копирование килобайта
				result = this->noexp(value / kb, onlyNum);
				// Добавляем наименование единицы измерения
				result.append(" Kb");
			// Если переданное значение соответствует байту
			} else {
				// Выполняем копирование байтов
				result = this->noexp(value, onlyNum);
				// Добавляем наименование единицы измерения
				result.append(" bytes");
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * sizeBuffer Метод получения размера буфера в байтах
 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
 * @return    размер буфера в байтах
 */
size_t awh::Framework::sizeBuffer(const string & str) const noexcept {
	/**
	 * Readme - http://www.securitylab.ru/analytics/243414.php
	 * 
	 * Example: 17520 Байт / .04 секунды = .44 МБ/сек = 3.5 Мб/сек
	 * Description: Пропускная способность = размер буфера / задержка
	 * 
	 * 1. Количество байт в киллобайте: 1024
	 * 2. Количество байт в мегабайте: 1024000
	 * 3. Количество байт в гигабайте: 1024000000
	 * 
	 * Размер буфера: 65536
	 * Задержка сети: .04
	 * Количество бит в байте: 8
	 * 
	 * 65536 / .04 / 1024000 = 1.6 (МБ/сек) * 8 = 13 Мб/сек
	 * 
	 * Получение размера буфера
	 * (13 / 8) * (1024000 * .04) = 66560
	 * 
	 */
	// Результат работы функции
	size_t result = 0;
	// Выполняем проверку входящей строки
	const auto & match = this->_regexp.exec(str, this->_buffers);
	// Если данные найдены
	if(!match.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Размерность скорости
			float dimension = .0f;
			// Получаем значение скорости
			const float speed = ::stof(match[1]);
			// Проверяем являются ли переданные данные байтами (8, 16, 32, 64, 128, 256, 512, 1024 ...)
			const bool bytes = !::fmod(speed / 8.f, 2.f);
			// Если это биты
			if(this->compare("bps", match[2]))
				// Выполняем установку множителя
				dimension = 1.f;
			// Если это размерность в киллобитах
			else if(this->compare("kbps", match[2]))
				// Выполняем установку множителя
				dimension = (bytes ? 1000.f : 1024.f);
			// Если это размерность в мегабитах
			else if(this->compare("Mbps", match[2]))
				// Выполняем установку множителя
				dimension = (bytes ? 1000000.f : 1024000.f);
			// Если это размерность в гигабитах
			else if(this->compare("Gbps", match[2]))
				// Выполняем установку множителя
				dimension = (bytes ? 1000000000.f : 1024000000.f);
			// Выполняем получение размера в байтах
			result = static_cast <size_t> ((speed / 8.f) * (dimension * .04f));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * Framework Конструктор
 */
awh::Framework::Framework() noexcept : _locale(AWH_LOCALE) {
	// Устанавливаем локализацию системы
	this->setLocale();
	// Устанавливаем регулярное выражение для парсинга буферов данных
	this->_buffers = this->_regexp.build("([\\d\\.\\,]+)\\s*(bps|kbps|Mbps|Gbps)$", {regexp_t::option_t::UTF8});
	// Устанавливаем регулярное выражение для парсинга байт
	this->_bytes = this->_regexp.build("([\\d\\.\\,]+)\\s*(bytes|b|Kb|Mb|Gb|Tb)$", {regexp_t::option_t::UTF8, regexp_t::option_t::CASELESS});
}
/**
 * Framework Конструктор
 * @param locale локализация приложения
 */
awh::Framework::Framework(const string & locale) noexcept {
	// Устанавливаем локализацию системы
	this->setLocale(locale);
	// Устанавливаем регулярное выражение для парсинга буферов данных
	this->_buffers = this->_regexp.build("([\\d\\.\\,]+)\\s*(bps|kbps|Mbps|Gbps)$", {regexp_t::option_t::UTF8});
	// Устанавливаем регулярное выражение для парсинга байт
	this->_bytes = this->_regexp.build("([\\d\\.\\,]+)\\s*(bytes|b|Kb|Mb|Gb|Tb)$", {regexp_t::option_t::UTF8, regexp_t::option_t::CASELESS});
}
