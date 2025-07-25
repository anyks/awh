/**
 * @file: fmk.hpp
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

#ifndef __AWH_FRAMEWORK__
#define __AWH_FRAMEWORK__

/**
 * Стандартные модули
 */
#include <set>
#include <map>
#include <list>
#include <cmath>
#include <bitset>
#include <chrono>
#include <locale>
#include <string>
#include <vector>
#include <limits>
#include <cctype>
#include <cwctype>
#include <cstdarg>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <cstdlib>
#include <codecvt>
#include <iostream>
#include <algorithm>
#include <type_traits>
#include <unordered_map>
#include <sys/types.h>

/**
 * Наши модули
 */
#include "os.hpp"
#include "../net/nwt.hpp"

/**
 * Для операционной системы не являющейся OS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Если используется модуль IDN
	 */
	#if AWH_IDN
		/**
		 * Модуль iconv
		 */
		#include <iconv/iconv.h>
	#endif
#endif

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Framework Класс фреймворка
	 */
	typedef class AWHSHARED_EXPORT Framework {
		public:
			/**
			 * Типы кодировок адресов файлов и каталогов
			 */
			enum class codepage_t : uint8_t {
				NONE        = 0x00, // Кодировка не установлена
				AUTO        = 0x01, // Автоматическое определение
				UTF8_CP1251 = 0x02, // Кодировка UTF-8
				CP1251_UTF8 = 0x03  // Кодировка CP1251
			};
			/**
			 * Флаги трансформации строк
			 */
			enum class transform_t : uint8_t {
				NONE  = 0x00, // Флаг не установлен
				TRIM  = 0x01, // Флаг удаления пробелов
				UPPER = 0x02, // Флаг перевода в верхний регистр
				LOWER = 0x03, // Флаг перевода в нижний регистр
				SMART = 0x04  // Флаг умного перевода начальных символов в верхний режим
			};
			/**
			 * Тип штампа времени
			 */
			enum class chrono_t : uint8_t {
				NONE         = 0x00, // Не установлено
				YEAR         = 0x01, // Год
				MONTH        = 0x02, // Месяц
				WEEK         = 0x03, // Неделя
				DAY          = 0x04, // День
				HOUR         = 0x05, // Час
				MINUTES      = 0x06, // Минуты
				SECONDS      = 0x07, // Секунды
				MILLISECONDS = 0x08, // Миллисекунды
				MICROSECONDS = 0x09, // Микросекунды
				NANOSECONDS  = 0x0A  // Наносекунды
			};
			/**
			 * Флаги проверки текстовых данных
			 */
			enum class check_t : uint8_t {
				NONE            = 0x00, // Флаг не установлен
				URL             = 0x01, // Флаг проверки на URL-адрес
				UTF8            = 0x02, // Флаг проверки на UTF-8
				PRINT           = 0x03, // Флаг проверки на печатаемый символ
				UPPER           = 0x04, // Флаг проверки на верхний регистр
				LOWER           = 0x05, // Флаг проверки на нижний регистр
				SPACE           = 0x06, // Флаг проверки на пробел
				LATIAN          = 0x07, // Флаг проверки на латинские символы
				NUMBER          = 0x08, // Флаг проверки на число
				DECIMAL         = 0x09, // Флаг проверки на число с плавающей точкой
				PSEUDO_NUMBER   = 0x0A, // Флаг проверки на псевдо-число
				PRESENCE_LATIAN = 0x0B  // Флаг проверки наличия латинских символов в строке
			};
		private:
			// Объект парсинга nwt адреса
			nwt_t _nwt;
		private:
			// Устанавливаем локаль по умолчанию
			locale _locale;
		private:
			// Объект регулярного выражения
			regexp_t _regexp;
		private:
			// Регулярное выражение для парсинга байт
			regexp_t::exp_t _bytes;
			// Регулярное выражение для парсинга буферов данных
			regexp_t::exp_t _buffers;
		public:
			/**
			 * findInMap Шаблон метода поиска в контейнере map указанного значения
			 * @tparam A тип контейнера
			 * @tparam B тип искомого значения
			 */
			template <typename A, typename B>
			/**
			 * findInMap Метод поиска в контейнере map указанного значения
			 * @param val значение которое необходимо найти
			 * @param map контейнер в котором нужно произвести поиск
			 * @return    итератор найденного элемента в контейнере
			 */
			typename A::const_iterator findInMap(const B & val, const A & map) const noexcept {
				// Если нам необходимо выполнить поиск по значению строке
				if(is_same <B, string>::value || is_same <B, wstring>::value){
					/**
					 * Структура для проверки данных
					 */
					struct Check {
						private:
							// Строка с которой нужно сравнить
							B _value;
						private:
							// Объект фреймворка
							const Framework * _fmk;
						private:
							/**
							 * compare Метод выполнения сравнения чисел
							 * @param a первое число для сравнения
							 * @param b второе число для сравнения
							 * @return  результат выполненной проверки
							 */
							bool compare(const B a, const B b) const noexcept {
								// Выполняем сравнение
								return (a != b);
							}
							/**
							 * compare Метод выполнения сравнения строк
							 * @param a первое число для сравнения
							 * @param b второе число для сравнения
							 * @return  результат выполненной проверки
							 */
							bool compare(const string & a, const string & b) const noexcept {
								// Выполняем сравнение
								return !this->_fmk->compare(a, b);
							}
							/**
							 * compare Метод выполнения сравнения строк
							 * @param a первое число для сравнения
							 * @param b второе число для сравнения
							 * @return  результат выполненной проверки
							 */
							bool compare(const wstring & a, const wstring & b) const noexcept {
								// Выполняем сравнение
								return !this->_fmk->compare(a, b);
							}
						public:
							/**
							 * Оператор [()] выполнения сравнения полученных данных
							 * @param item текущее проверяемое значение
							 * @return     результат проверки
							 */
							bool operator () (const typename A::value_type & item) const noexcept {
								// Выполняем сравнение текущего полученного значения
								return this->compare(this->_value, item.second); 
							}
						public:
							/**
							 * Check Конструктор
							 * @param value эталонное значение для стравнения
							 * @param fmk   объект фреймворка
							 */
							Check(const B & value, const Framework * fmk) noexcept : _value(value), _fmk(fmk) {}
					} callback(val, this);
					// Выполняем поиск искомого значения в контейнере map
					return find_if_not(map.cbegin(), map.cend(), callback);
				// Если нам необходимо выполнить поиск по статическому типу данных
				} else {
					/**
					 * Структура для проверки данных
					 */
					struct Check {
						private:
							// Строка с которой нужно сравнить
							B _value;
						public:
							/**
							 * Оператор [()] выполнения сравнения полученных данных
							 * @param item текущее проверяемое значение
							 * @return     результат проверки
							 */
							bool operator () (const typename A::value_type & item) const noexcept {
								// Выполняем сравнение текущего полученного значения
								return (item.second != this->_value); 
							}
						public:
							/**
							 * Check Конструктор
							 * @param value эталонное значение для стравнения
							 */
							Check(const B & value) noexcept : _value(value) {}
					};
					// Выполняем поиск искомого значения в контейнере map
					return find_if_not(map.cbegin(), map.cend(), Check(val));
				}
			}
		public:
			/**
			 * is Метод проверки текста на соответствие флагу
			 * @param text текст для проверки
			 * @param flag флаг проверки
			 * @return     результат проверки
			 */
			bool is(const char letter, const check_t flag) const noexcept;
			/**
			 * is Метод проверки текста на соответствие флагу
			 * @param text текст для проверки
			 * @param flag флаг проверки
			 * @return     результат проверки
			 */
			bool is(const wchar_t letter, const check_t flag) const noexcept;
		public:
			/**
			 * is Метод проверки текста на соответствие флагу
			 * @param text текст для проверки
			 * @param flag флаг проверки
			 * @return     результат проверки
			 */
			bool is(const string & text, const check_t flag) const noexcept;
			/**
			 * is Метод проверки текста на соответствие флагу
			 * @param text текст для проверки
			 * @param flag флаг проверки
			 * @return     результат проверки
			 */
			bool is(const wstring & text, const check_t flag) const noexcept;
		public:
			/**
			 * compare Метод сравнения двух строк без учёта регистра
			 * @param first  первое слово
			 * @param second второе слово
			 * @return       результат сравнения
			 */
			bool compare(const string & first, const string & second) const noexcept;
			/**
			 * compare Метод сравнения двух строк без учёта регистра
			 * @param first  первое слово
			 * @param second второе слово
			 * @return       результат сравнения
			 */
			bool compare(const wstring & first, const wstring & second) const noexcept;
		private:
			/**
			 * timestamp Метод получения штампа времени в указанных единицах измерения
			 * @param buffer буфер бинарных данных для установки штампа времени
			 * @param size   размер бинарных данных штампа времени
			 * @param type   тип формируемого штампа времени
			 * @param text   флаг извлечения данных в текстовом виде
			 */
			void timestamp(void * buffer, const size_t size, const chrono_t type, const bool text) const noexcept;
		public:
			/**
			 * timestamp Шаблон метода получения штампа времени в указанных единицах измерения
			 * @tparam T тип данных в котором извлекаются данные
			 */
			template <typename T>
			/**
			 * timestamp Метод получения штампа времени в указанных единицах измерения
			 * @param type тип формируемого штампа времени
			 * @return     сгенерированный штамп времени
			 */
			T timestamp(const chrono_t type) const noexcept {
				// Результат работы функции
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
				this->timestamp(&result, sizeof(result), type, is_class_v <T>);
				// Выводим полученный результат
				return result;
			}
		public:
			/**
			 * iconv Метод конвертирования строки кодировки
			 * @param text     текст для конвертирования
			 * @param codepage кодировка в которую необходимо сконвертировать текст
			 * @return         сконвертированный текст в требуемой кодировке
			 */
			string iconv(const string & text, const codepage_t codepage = codepage_t::AUTO) const noexcept;
		public:
			/**
			 * transform Метод трансформации одного символа
			 * @param letter символ для трансформации
			 * @param flag   флаг трансформации
			 * @return       трансформированный символ
			 */
			char transform(char letter, const transform_t flag) const noexcept;
			/**
			 * transform Метод трансформации одного символа
			 * @param letter символ для трансформации
			 * @param flag   флаг трансформации
			 * @return       трансформированный символ
			 */
			wchar_t transform(wchar_t letter, const transform_t flag) const noexcept;
		public:
			/**
			 * transform Метод трансформации строки
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 */
			string & transform(string & text, const transform_t flag) const noexcept;
			/**
			 * transform Метод трансформации строки
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 */
			wstring & transform(wstring & text, const transform_t flag) const noexcept;
		public:
			/**
			 * transform Метод трансформации строки
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 */
			const string & transform(const string & text, const transform_t flag) const noexcept;
			/**
			 * transform Метод трансформации строки
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 */
			const wstring & transform(const wstring & text, const transform_t flag) const noexcept;
		public:
			/**
			 * join Метод объединения списка строк в одну строку
			 * @param items список строк которые необходимо объединить
			 * @param delim разделитель
			 * @return      строка полученная после объединения
			 */
			string join(const vector <string> & items, const string & delim) const noexcept;
			/**
			 * join Метод объединения списка строк в одну строку
			 * @param items список строк которые необходимо объединить
			 * @param delim разделитель
			 * @return      строка полученная после объединения
			 */
			wstring join(const vector <wstring> & items, const wstring & delim) const noexcept;
		public:
			/**
			 * split Метод разделения строк на токены
			 * @param text      строка для парсинга
			 * @param delim     разделитель
			 * @param container результирующий вектор
			 */
			vector <string> & split(const string & text, const string & delim, vector <string> & container) const noexcept;
			/**
			 * split Метод разделения строк на токены
			 * @param text      строка для парсинга
			 * @param delim     разделитель
			 * @param container результирующий вектор
			 */
			vector <wstring> & split(const wstring & text, const wstring & delim, vector <wstring> & container) const noexcept;
		public:
			/**
			 * convert Метод конвертирования строки utf-8 в строку
			 * @param str строка utf-8 для конвертирования
			 * @return    обычная строка
			 */
			string convert(const wstring & str) const noexcept;
			/**
			 * convert Метод конвертирования строки в строку utf-8
			 * @param str строка для конвертирования
			 * @return    строка в utf-8
			 */
			wstring convert(const string & str) const noexcept;
		public:
			/**
			 * Шаблон функции определения точного размера, сколько занимает число байт
			 * @tparam T тип данных с которым работает функция
			 */
			template <typename T>
			/**
			 * size Метод определения точного размера, сколько занимает число байт
			 * @param num число для проверки
			 * @return    фактический размер занимаемым числом байт
			 */
			size_t size(const T num) const noexcept {
				// Если данные являются основными
				if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value)
					// Выполняем подсчёт занимаемых числом данных
					return this->size(&num, sizeof(num));
				// Выводим значение по умолчанию
				return 0;
			}
			/**
			 * size Метод определения точного размера, сколько занимают данные (в байтах) в буфере
			 * @param value значение бинарного буфера для проверки
			 * @param size  общий размер бинарного буфера
			 * @return      фактический размер буфера занимаемый данными
			 */
			size_t size(const void * value, const size_t size) const noexcept;
		public:
			/**
			 * Шаблон функции проверки больше первое число второго или нет (бинарным методом)
			 * @tparam T тип данных с которым работает функция
			 */
			template <typename T>
			/**
			 * greater Метод проверки больше первое число второго или нет (бинарным методом)
			 * @param num1 значение первого числа в бинарном виде
			 * @param num2 значение второго числа в бинарном виде
			 * @return     результат проверки
			 */
			bool greater(const T num1, const T num2) const noexcept {
				// Если данные являются основными
				if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value)
					// Выполняем проверку
					return this->greater(&num1, &num2, sizeof(num1));
				// Выводим значение по умолчанию
				return false;
			}
			/**
			 * greater Метод проверки больше первое число второго или нет (бинарным методом)
			 * @param value1 значение первого числа в бинарном виде
			 * @param value2 значение второго числа в бинарном виде
			 * @param size   размер бинарного буфера числа
			 * @return       результат проверки
			 */
			bool greater(const void * value1, const void * value2, const size_t size) const noexcept;
		public:
			/**
			 * Шаблон функции конвертации чисел в указанную систему счисления
			 * @tparam T тип данных с которым работает функция
			 */
			template <typename T>
			/**
			 * itoa Метод конвертации чисел в указанную систему счисления
			 * @param value число для конвертации
			 * @param radix система счисления
			 * @return      полученная строка в указанной системе счисления
			 */
			string itoa(const T value, const uint8_t radix) const noexcept {
				// Если данные являются основными
				if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value)
					// Выполняем конвертацию чисел в указанную систему счисления
					return this->itoa(&value, sizeof(value), radix);
				// Выводим пустое значение
				return "";
			}
			/**
			 * itoa Метод конвертации чисел в указанную систему счисления
			 * @param value бинарный буфер числа для конвертации
			 * @param size  размер бинарного буфера
			 * @param radix система счисления
			 * @return      полученная строка в указанной системе счисления
			 */
			string itoa(const void * value, const size_t size, const uint8_t radix) const noexcept;
		public:
			/**
			 * Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 * @tparam T тип данных с которым работает функция
			 */
			template <typename T>
			/**
			 * atoi Метод конвертации строковых чисел в десятичную систему счисления
			 * @param value число в бинарном виде для конвертации в 10-ю систему
			 * @param radix система счисления
			 * @return      полученное значение в десятичной системе счисления
			 */
			T atoi(const string & value, const uint8_t radix) const noexcept {
				// Результат работы функции
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
				this->atoi(value, radix, &result, sizeof(result));
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
			void atoi(const string & value, const uint8_t radix, void * buffer, const size_t size) const noexcept;
		public:
			/**
			 * noexp Метод перевода числа в безэкспоненциальную форму
			 * @param number число для перевода
			 * @param step   размер шага после запятой
			 * @return       число в безэкспоненциальной форме
			 */
			string noexp(const double number, const uint8_t step) const noexcept;
			/**
			 * noexp Метод перевода числа в безэкспоненциальную форму
			 * @param number  число для перевода
			 * @param onlyNum выводить только числа
			 * @return        число в безэкспоненциальной форме
			 */
			string noexp(const double number, const bool onlyNum = false) const noexcept;
		public:
			/**
			 * rate Метод порверки на сколько процентов (A > B) или (A < B)
			 * @param a первое число
			 * @param b второе число
			 * @return  результат расчёта
			 */
			float rate(const float a, const float b) const noexcept;
			/**
			 * floor Метод приведения количества символов после запятой к указанному количества
			 * @param x число для приведения
			 * @param n количество символов после запятой
			 * @return  сформированное число
			 */
			double floor(const double x, const uint8_t n) const noexcept;
		public:
			/**
			 * rome2arabic Метод перевода римских цифр в арабские
			 * @param word римское число
			 * @return     арабское число
			 */
			uint16_t rome2arabic(const string & word) const noexcept;
			/**
			 * rome2arabic Метод перевода римских цифр в арабские
			 * @param word римское число
			 * @return     арабское число
			 */
			uint16_t rome2arabic(const wstring & word) const noexcept;
		public:
			/**
			 * arabic2rome Метод перевода арабских чисел в римские
			 * @param number арабское число от 1 до 4999
			 * @return       римское число
			 */
			wstring arabic2rome(const uint32_t number) const noexcept;
			/**
			 * arabic2rome Метод перевода арабских чисел в римские
			 * @param word арабское число от 1 до 4999
			 * @return     римское число
			 */
			string arabic2rome(const string & word) const noexcept;
			/**
			 * arabic2rome Метод перевода арабских чисел в римские
			 * @param word арабское число от 1 до 4999
			 * @return     римское число
			 */
			wstring arabic2rome(const wstring & word) const noexcept;
		public:
			/**
			 * setCase Метод запоминания регистра слова
			 * @param pos   позиция для установки регистра
			 * @param start начальное значение регистра в бинарном виде
			 * @return      позиция верхнего регистра в бинарном виде
			 */
			uint64_t setCase(const uint64_t pos, const uint64_t start = 0) const noexcept;
			/**
			 * countLetter Метод подсчёта количества указанной буквы в слове
			 * @param word   слово в котором нужно подсчитать букву
			 * @param letter букву которую нужно подсчитать
			 * @return       результат подсчёта
			 */
			size_t countLetter(const wstring & word, const wchar_t letter) const noexcept;
		public:
			/**
			 * format Метод реализации функции формирования форматированной строки
			 * @param format формат строки вывода
			 * @param args   передаваемые аргументы
			 * @return       сформированная строка
			 */
			string format(const char * format, ...) const noexcept;
			/**
			 * format Метод реализации функции формирования форматированной строки
			 * @param format формат строки вывода
			 * @param items  список аргументов строки
			 * @return       сформированная строка
			 */
			string format(const string & format, const vector <string> & items) const noexcept;
		public:
			/**
			 * exists Метод проверки существования слова в тексте
			 * @param word слово для проверки
			 * @param text текст в котором выполнения проверка
			 * @return     результат выполнения проверки
			 */
			bool exists(const string & word, const string & text) const noexcept;
			/**
			 * exists Метод проверки существования слова в тексте
			 * @param word слово для проверки
			 * @param text текст в котором выполнения проверка
			 * @return     результат выполнения проверки
			 */
			bool exists(const wstring & word, const wstring & text) const noexcept;
		public:
			/**
			 * replace Метод замены в тексте слово на другое слово
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 */
			string & replace(string & text, const string & word, const string & alt = "") const noexcept;
			/**
			 * replace Метод замены в тексте слово на другое слово
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 */
			wstring & replace(wstring & text, const wstring & word, const wstring & alt = L"") const noexcept;
		public:
			/**
			 * replace Метод замены в тексте слово на другое слово
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 */
			const string & replace(const string & text, const string & word, const string & alt = "") const noexcept;
			/**
			 * replace Метод замены в тексте слово на другое слово
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 */
			const wstring & replace(const wstring & text, const wstring & word, const wstring & alt = L"") const noexcept;
		public:
			/**
			 * kv Метод извлечения ключей и значений из текста
			 * @param text      текст из которого извлекаются записи
			 * @param delim     разделитель записей
			 * @param separator разделитель ключа и значения
			 * @param escaping  символы экранирования
			 * @return          список найденных элементов
			 */
			std::unordered_map <string, string> kv(const string & text, const string & delim, const string & separator = "=", const vector <string> & escaping = {string{"\""}}) const noexcept;
			/**
			 * kv Метод извлечения ключей и значений из текста
			 * @param text      текст из которого извлекаются записи
			 * @param delim     разделитель записей
			 * @param separator разделитель ключа и значения
			 * @param escaping  символы экранирования
			 * @return          список найденных элементов
			 */
			std::unordered_map <wstring, wstring> kv(const wstring & text, const wstring & delim, const wstring & separator = L"=", const vector <wstring> & escaping = {wstring{L"\""}}) const noexcept;
		public:
			/**
			 * domainZone Метод установки пользовательской зоны
			 * @param zone пользовательская зона
			 */
			void domainZone(const string & zone) noexcept;
			/**
			 * domainZones Метод установки списка пользовательских зон
			 * @param zones список доменных зон интернета
			 */
			void domainZones(const std::set <string> & zones) noexcept;
			/**
			 * domainZones Метод извлечения списка пользовательских зон интернета
			 * @return список доменных зон
			 */
			const std::set <string> & domainZones() const noexcept;
		public:
			/**
			 * setLocale Метод установки системной локали
			 * @param locale локализация приложения
			 */
			void setLocale(const string & locale = AWH_LOCALE) noexcept;
		public:
			/**
			 * urls Метод извлечения координат url адресов в строке
			 * @param text текст для извлечения url адресов
			 * @return     список координат с url адресами
			 */
			std::map <size_t, size_t> urls(const string & text) const noexcept;
		public:
			/**
			 * icon Метод получения иконки
			 * @param end флаг завершения работы
			 * @return    иконка напутствия работы
			 */
			string icon(const bool end = false) const noexcept;
		public:
			/**
			 * bytes Метод получения размера в байтах из строки
			 * @param str строка обозначения размерности (b, Kb, Mb, Gb, Tb)
			 * @return    размер в байтах
			 */
			double bytes(const string & str) const noexcept;
			/**
			 * bytes Метод конвертации байт в строку
			 * @param value   количество байт
			 * @param onlyNum выводить только числа
			 * @return        полученная строка
			 */
			string bytes(const double value, const bool onlyNum = false) const noexcept;
		public:
			/**
			 * sizeBuffer Метод получения размера буфера в байтах
			 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
			 * @return    размер буфера в байтах
			 */
			size_t sizeBuffer(const string & str) const noexcept;
		public:
			/**
			 * Framework Конструктор
			 */
			Framework() noexcept;
			/**
			 * Framework Конструктор
			 * @param locale локализация приложения
			 */
			Framework(const string & locale) noexcept;
			/**
			 * ~Framework Деструктор
			 */
			~Framework() noexcept {}
	} fmk_t;
};

#endif // __AWH_FRAMEWORK__
