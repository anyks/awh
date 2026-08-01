/**
 * @file: fmk.hpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл ядра фреймворка — класс Framework с базовыми утилитами библиотеки:
 *        работа со строками и кодировками, регистр символов, форматирование, конвертация типов,
 *        проверка форматов данных, разбор чисел и вспомогательные операции над контейнерами
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_FRAMEWORK__
#define __AWH_FRAMEWORK__

/**
 * Стандартные заголовочные файлы
 */
#include <locale>
#include <string>
#include <vector>
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <cstdarg>
#include <cstdlib>
#include <functional>
#include <type_traits>
#include <unordered_set>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "lib.hpp"
#include "../net/nwt.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Прототип класса работы с логами
	 *
	 */
	class Logging;

	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс фреймворка
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Framework {
		public:
			/**
			 * @brief Типы кодировок адресов файлов и каталогов
			 *
			 */
			enum class codepage_t : uint8_t {
				NONE        = 0x00, // Кодировка не установлена
				AUTO        = 0x01, // Автоматическое определение
				UTF8_CP1251 = 0x02, // Кодировка UTF-8
				CP1251_UTF8 = 0x03  // Кодировка CP1251
			};
			/**
			 * @brief Флаги трансформации строк
			 *
			 */
			enum class transform_t : uint8_t {
				NONE       = 0x00, // Флаг не установлен
				TRIM       = 0x01, // Флаг удаления пробелов
				UPPER_CASE = 0x02, // Флаг перевода в верхний регистр
				LOWER_CASE = 0x03, // Флаг перевода в нижний регистр
				SMART_CASE = 0x04  // Флаг умного перевода начальных символов в верхний режим
			};
			/**
			 * @brief Тип штампа времени
			 *
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
			 * @brief Флаги проверки текстовых данных
			 *
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
			std::locale _locale;
		private:
			// Объект работы с логами
			const Logging * _log;
		public:
			/**
			 * @brief Шаблон метода поиска в контейнере map указанного значения
			 *
			 * @tparam A тип контейнера
			 * @tparam B тип искомого значения
			 *
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод поиска в контейнере map указанного значения
			 *
			 * @param val значение которое необходимо найти
			 * @param map контейнер в котором нужно произвести поиск
			 * @return    итератор найденного элемента в контейнере
			 *
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
							 * @brief Метод выполнения сравнения чисел
							 *
							 * @param a первое число для сравнения
							 * @param b второе число для сравнения
							 * @return  результат выполненной проверки
							 *
							 */
							bool compare(const B a, const B b) const noexcept {
								// Выполняем сравнение
								return (a != b);
							}
							/**
							 * @brief Метод выполнения сравнения строк
							 *
							 * @param a первое число для сравнения
							 * @param b второе число для сравнения
							 * @return  результат выполненной проверки
							 *
							 */
							bool compare(string_view a, string_view b) const noexcept {
								// Выполняем сравнение
								return !this->_fmk->compare(a, b);
							}
							/**
							 * @brief Метод выполнения сравнения строк
							 *
							 * @param a первое число для сравнения
							 * @param b второе число для сравнения
							 * @return  результат выполненной проверки
							 *
							 */
							bool compare(wstring_view a, wstring_view b) const noexcept {
								// Выполняем сравнение
								return !this->_fmk->compare(a, b);
							}
						public:
							/**
							 * @brief Оператор [()] выполнения сравнения полученных данных
							 *
							 * @param item текущее проверяемое значение
							 * @return     результат проверки
							 *
							 */
							bool operator () (const typename A::value_type & item) const noexcept {
								// Выполняем сравнение текущего полученного значения
								return this->compare(this->_value, item.second);
							}
						public:
							/**
							 * @brief Конструктор
							 *
							 * @param value эталонное значение для стравнения
							 * @param fmk   объект фреймворка
							 *
							 */
							Check(const B & value, const Framework * fmk) noexcept : _value(value), _fmk(fmk) {}
					} callback(val, this);
					// Выполняем поиск искомого значения в контейнере map
					return std::find_if_not(map.cbegin(), map.cend(), callback);
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
							 * @brief Оператор [()] выполнения сравнения полученных данных
							 *
							 * @param item текущее проверяемое значение
							 * @return     результат проверки
							 *
							 */
							bool operator () (const typename A::value_type & item) const noexcept {
								// Выполняем сравнение текущего полученного значения
								return (item.second != this->_value);
							}
						public:
							/**
							 * @brief Конструктор
							 *
							 * @param value эталонное значение для стравнения
							 *
							 */
							Check(const B & value) noexcept : _value(value) {}
					};
					// Выполняем поиск искомого значения в контейнере map
					return std::find_if_not(map.cbegin(), map.cend(), Check(val));
				}
			}
		public:
			/**
			 * @brief Метод генерации уникального идентификатора
			 * 
			 * @return уникальный идентификатор
			 *
			 */
			uint32_t identifier() const noexcept;
		public:
			/**
			 * @brief Метод проверки текста на соответствие флагу
			 *
			 * @param letter текст для проверки
			 * @param flag   флаг проверки
			 * @return       результат проверки
			 *
			 */
			bool is(const char letter, const check_t flag) const noexcept;
			/**
			 * @brief Метод проверки текста на соответствие флагу
			 *
			 * @param letter текст для проверки
			 * @param flag   флаг проверки
			 * @return       результат проверки
			 *
			 */
			bool is(const wchar_t letter, const check_t flag) const noexcept;
		public:
			/**
			 * @brief Метод проверки текста на соответствие флагу
			 *
			 * @param text текст для проверки
			 * @param flag флаг проверки
			 * @return     результат проверки
			 *
			 */
			bool is(string_view text, const check_t flag) const noexcept;
			/**
			 * @brief Метод проверки текста на соответствие флагу
			 *
			 * @param text текст для проверки
			 * @param flag флаг проверки
			 * @return     результат проверки
			 *
			 */
			bool is(wstring_view text, const check_t flag) const noexcept;
		public:
			/**
			 * @brief Метод сравнения двух строк без учёта регистра
			 *
			 * @param first  первое слово
			 * @param second второе слово
			 * @return       результат сравнения
			 *
			 */
			bool compare(string_view first, string_view second) const noexcept;
			/**
			 * @brief Метод сравнения двух строк без учёта регистра
			 *
			 * @param first  первое слово
			 * @param second второе слово
			 * @return       результат сравнения
			 *
			 */
			bool compare(const char * first, const char * second) const noexcept;
		public:
			/**
			 * @brief Метод сравнения двух строк без учёта регистра
			 *
			 * @param first  первое слово
			 * @param second второе слово
			 * @return       результат сравнения
			 *
			 */
			bool compare(wstring_view first, wstring_view second) const noexcept;
			/**
			 * @brief Метод сравнения двух строк без учёта регистра
			 *
			 * @param first  первое слово
			 * @param second второе слово
			 * @return       результат сравнения
			 *
			 */
			bool compare(const wchar_t * first, const wchar_t * second) const noexcept;
		private:
			/**
			 * @brief Метод получения штампа времени в указанных единицах измерения
			 *
			 * @param buffer буфер бинарных данных для установки штампа времени
			 * @param size   размер бинарных данных штампа времени
			 * @param type   тип формируемого штампа времени
			 * @param text   флаг извлечения данных в текстовом виде
			 *
			 */
			void timestamp(void * buffer, const size_t size, const chrono_t type, const bool text) const noexcept;
		public:
			/**
			 * @brief Шаблон метода получения штампа времени в указанных единицах измерения
			 *
			 * @tparam T тип данных в котором извлекаются данные
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод получения штампа времени в указанных единицах измерения
			 *
			 * @param type тип формируемого штампа времени
			 * @return     сгенерированный штамп времени
			 *
			 */
			T timestamp(const chrono_t type) const noexcept;
		public:
			/**
			 * @brief Метод конвертирования строки кодировки
			 *
			 * @param text     текст для конвертирования
			 * @param codepage кодировка в которую необходимо сконвертировать текст
			 * @return         сконвертированный текст в требуемой кодировке
			 *
			 */
			string transcode(string_view text, const codepage_t codepage = codepage_t::AUTO) const noexcept;
		public:
			/**
			 * @brief Метод трансформации одного символа
			 *
			 * @param letter символ для трансформации
			 * @param flag   флаг трансформации
			 * @return       трансформированный символ
			 *
			 */
			char transform(const char letter, const transform_t flag) const noexcept;
			/**
			 * @brief Метод трансформации одного символа
			 *
			 * @param letter символ для трансформации
			 * @param flag   флаг трансформации
			 * @return       трансформированный символ
			 *
			 */
			wchar_t transform(const wchar_t letter, const transform_t flag) const noexcept;
		public:
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			string & transform(string & text, const transform_t flag) const noexcept;
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			wstring & transform(wstring & text, const transform_t flag) const noexcept;
		public:
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			const string & transform(const string & text, const transform_t flag) const noexcept;
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			const wstring & transform(const wstring & text, const transform_t flag) const noexcept;
		public:
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			string transform(string_view text, const transform_t flag) const noexcept;
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			wstring transform(wstring_view text, const transform_t flag) const noexcept;
		public:
			/**
			 * @brief Метод объединения списка строк в одну строку
			 *
			 * @param items список строк которые необходимо объединить
			 * @param delim разделитель
			 * @return      строка полученная после объединения
			 *
			 */
			string join(const vector <string> & items, string_view delim) const noexcept;
			/**
			 * @brief Метод объединения списка строк в одну строку
			 *
			 * @param items список строк которые необходимо объединить
			 * @param delim разделитель
			 * @return      строка полученная после объединения
			 *
			 */
			wstring join(const vector <wstring> & items, wstring_view delim) const noexcept;
		public:
			/**
			 * @brief Метод разделения строк на токены
			 *
			 * @param text      строка для парсинга
			 * @param delim     разделитель
			 * @param container результирующий вектор
			 *
			 */
			vector <string> & split(string_view text, string_view delim, vector <string> & container) const noexcept;
			/**
			 * @brief Метод разделения строк на токены
			 *
			 * @param text      строка для парсинга
			 * @param delim     разделитель
			 * @param container результирующий вектор
			 *
			 */
			vector <wstring> & split(wstring_view text, wstring_view delim, vector <wstring> & container) const noexcept;
		public:
			/**
			 * @brief Метод конвертирования строки в строку utf-8
			 *
			 * @param str строка для конвертирования
			 * @return    строка в utf-8
			 *
			 */
			wstring convert(string_view str) const noexcept;
			/**
			 * @brief Метод конвертирования строки utf-8 в строку
			 *
			 * @param str строка utf-8 для конвертирования
			 * @return    обычная строка
			 *
			 */
			string convert(wstring_view str) const noexcept;
			/**
			 * @brief Метод конвертирования строки в строку utf-8
			 *
			 * @param str строка для конвертирования
			 * @return    строка в utf-8
			 *
			 */
			wstring convert(const char * str) const noexcept;
			/**
			 * @brief Метод конвертирования строки utf-8 в строку
			 *
			 * @param str строка utf-8 для конвертирования
			 * @return    обычная строка
			 *
			 */
			string convert(const wchar_t * str) const noexcept;
			/**
			 * @brief Метод конвертирования строки в строку utf-8
			 *
			 * @param str строка для конвертирования
			 * @return    строка в utf-8
			 *
			 */
			wstring convert(const string & str) const noexcept;
			/**
			 * @brief Метод конвертирования строки utf-8 в строку
			 *
			 * @param str строка utf-8 для конвертирования
			 * @return    обычная строка
			 *
			 */
			string convert(const wstring & str) const noexcept;
		public:
			/**
			 * @brief функции определения точного размера, сколько занимает число байт
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод определения точного размера, сколько занимает число байт
			 *
			 * @param num число для проверки
			 * @return    фактический размер занимаемым числом байт
			 *
			 */
			size_t size(const T num) const noexcept;
			/**
			 * @brief Метод определения точного размера, сколько занимают данные (в байтах) в буфере
			 *
			 * @param value значение бинарного буфера для проверки
			 * @param size  общий размер бинарного буфера
			 * @return      фактический размер буфера занимаемый данными
			 *
			 */
			size_t size(const void * value, const size_t size) const noexcept;
		public:
			/**
			 * @brief Шаблон функции проверки больше первое число второго или нет (бинарным методом)
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод проверки больше первое число второго или нет (бинарным методом)
			 *
			 * @param num1 значение первого числа в бинарном виде
			 * @param num2 значение второго числа в бинарном виде
			 * @return     результат проверки
			 *
			 */
			bool isGreater(const T num1, const T num2) const noexcept;
			/**
			 * @brief Метод проверки больше первое число второго или нет (бинарным методом)
			 *
			 * @details Сличаются **числа**, лежащие в памяти так, как их кладёт сама
			 *          машина. Перебор идёт с последнего байта: на машине с обратным
			 *          порядком байт старший разряд числа лежит именно там. Так и
			 *          задумано, и разворачивать перебор не следует - иначе метод
			 *          перестанет сличать числа
			 *
			 * @warning Двоичный буфер, числом не являющийся, сличать этим методом
			 *          **нельзя**: сетевой адрес, аппаратный адрес, отпечаток - всё
			 *          это лежит в своём порядке, старшим байтом вперёд, и порядок их
			 *          даёт побайтное сличение `memcmp`, а не этот метод
			 *
			 * @param value1 значение первого числа в бинарном виде
			 * @param value2 значение второго числа в бинарном виде
			 * @param size   размер бинарного буфера числа
			 * @return       результат проверки
			 *
			 */
			bool isGreater(const void * value1, const void * value2, const size_t size) const noexcept;
		public:
			/**
			 * @brief Шаблон функции конвертации чисел в указанную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации чисел в указанную систему счисления
			 *
			 * @param value число для конвертации
			 * @param radix система счисления
			 * @return      полученная строка в указанной системе счисления
			 *
			 */
			string itoa(const T value, const uint8_t radix) const noexcept;
			/**
			 * @brief Метод конвертации чисел в указанную систему счисления
			 *
			 * @param value бинарный буфер числа для конвертации
			 * @param size  размер бинарного буфера
			 * @param radix система счисления
			 * @return      полученная строка в указанной системе счисления
			 *
			 */
			string itoa(const void * value, const size_t size, const uint8_t radix) const noexcept;
		public:
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value строковое представление числа
			 * @return      числовое значение в десятичной системе счисления
			 *
			 */
			T atoi(string_view value) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value строковое представление числа
			 * @return      числовое значение в десятичной системе счисления
			 *
			 */
			T atoi(const string & value) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  строковое представление числа
			 * @param length длина строки
			 * @return       числовое значение в десятичной системе счисления
			 *
			 */
			T atoi(const char * value, const size_t length) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value число в бинарном виде для конвертации в 10-ю систему
			 * @param radix система счисления
			 * @return      полученное значение в десятичной системе счисления
			 *
			 */
			T atoi(string_view value, const uint8_t radix) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value число в бинарном виде для конвертации в 10-ю систему
			 * @param radix система счисления
			 * @return      полученное значение в десятичной системе счисления
			 *
			 */
			T atoi(const string & value, const uint8_t radix) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  буфер числа в бинарном виде для конвертации в 10-ю систему
			 * @param length длина буфера числа в бинарном виде
			 * @param radix  система счисления
			 * @return       полученное значение в десятичной системе счисления
			 *
			 */
			T atoi(const char * value, const size_t length, const uint8_t radix) const noexcept;
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  число в бинарном виде для конвертации в 10-ю систему
			 * @param radix  система счисления
			 * @param buffer бинарный буфер куда следует положить результат
			 * @param size   размер бинарного буфера куда следует положить результат
			 *
			 */
			void atoi(string_view value, const uint8_t radix, void * buffer, const size_t size) const noexcept;
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  число в бинарном виде для конвертации в 10-ю систему
			 * @param radix  система счисления
			 * @param buffer бинарный буфер куда следует положить результат
			 * @param size   размер бинарного буфера куда следует положить результат
			 *
			 */
			void atoi(const string & value, const uint8_t radix, void * buffer, const size_t size) const noexcept;
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  число в бинарном виде для конвертации в 10-ю систему
			 * @param length длина буфера числа в бинарном виде
			 * @param radix  система счисления
			 * @param buffer бинарный буфер куда следует положить результат
			 * @param size   размер бинарного буфера куда следует положить результат
			 *
			 */
			void atoi(const char * value, const size_t length, const uint8_t radix, void * buffer, const size_t size) const noexcept;
		public:
			public:
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value строковое представление числа
			 * @return      числовое значение в десятичной системе счисления
			 *
			 */
			T atoi(wstring_view value) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value строковое представление числа
			 * @return      числовое значение в десятичной системе счисления
			 *
			 */
			T atoi(const wstring & value) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  строковое представление числа
			 * @param length длина строки
			 * @return       числовое значение в десятичной системе счисления
			 *
			 */
			T atoi(const wchar_t * value, const size_t length) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value число в бинарном виде для конвертации в 10-ю систему
			 * @param radix система счисления
			 * @return      полученное значение в десятичной системе счисления
			 *
			 */
			T atoi(wstring_view value, const uint8_t radix) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value число в бинарном виде для конвертации в 10-ю систему
			 * @param radix система счисления
			 * @return      полученное значение в десятичной системе счисления
			 *
			 */
			T atoi(const wstring & value, const uint8_t radix) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  буфер числа в бинарном виде для конвертации в 10-ю систему
			 * @param length длина буфера числа в бинарном виде
			 * @param radix  система счисления
			 * @return       полученное значение в десятичной системе счисления
			 *
			 */
			T atoi(const wchar_t * value, const size_t length, const uint8_t radix) const noexcept;
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  число в бинарном виде для конвертации в 10-ю систему
			 * @param radix  система счисления
			 * @param buffer бинарный буфер куда следует положить результат
			 * @param size   размер бинарного буфера куда следует положить результат
			 *
			 */
			void atoi(wstring_view value, const uint8_t radix, void * buffer, const size_t size) const noexcept;
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  число в бинарном виде для конвертации в 10-ю систему
			 * @param radix  система счисления
			 * @param buffer бинарный буфер куда следует положить результат
			 * @param size   размер бинарного буфера куда следует положить результат
			 *
			 */
			void atoi(const wstring & value, const uint8_t radix, void * buffer, const size_t size) const noexcept;
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  число в бинарном виде для конвертации в 10-ю систему
			 * @param length длина буфера числа в бинарном виде
			 * @param radix  система счисления
			 * @param buffer бинарный буфер куда следует положить результат
			 * @param size   размер бинарного буфера куда следует положить результат
			 *
			 */
			void atoi(const wchar_t * value, const size_t length, const uint8_t radix, void * buffer, const size_t size) const noexcept;
		public:
			/**
			 * @brief Метод перевода числа в безэкспоненциальную форму
			 *
			 * @details Количество знаков после запятой задаётся размером шага, дробная
			 *          часть при этом округляется. Целое число записывается без дробной
			 *          части вовсе, каким бы ни был размер шага.
			 *
			 *          @code{.cpp}
			 *          fmk.noexp(2986.808299, static_cast <uint8_t> (3));  // «2986.808»
			 *          fmk.noexp(2986.808299, static_cast <uint8_t> (4));  // «2986.8083»
			 *          fmk.noexp(2986., static_cast <uint8_t> (4));        // «2986»
			 *          @endcode
			 *
			 * @note Запись не зависит от установленной локали: разделителем дробной части
			 *       всегда служит точка, разделителей разрядов запись не содержит
			 *
			 * @note Нулевой размер шага даёт запись «0»: округлять до нуля знаков после
			 *       запятой следует перегрузкой с подбором точности
			 *
			 * @param number число для перевода
			 * @param step   размер шага после запятой
			 * @return       число в безэкспоненциальной форме
			 *
			 * @see noexp(const double, const bool)
			 *
			 */
			string noexp(const double number, const uint8_t step) const noexcept;
			/**
			 * @brief Метод перевода числа в безэкспоненциальную форму
			 *
			 * @details Количество знаков после запятой подбирается наименьшим из тех, при
			 *          котором запись читается обратно ровно тем же числом. Запись выходит
			 *          краткой, не теряя при этом ни одного значащего разряда.
			 *
			 *          @code{.cpp}
			 *          fmk.noexp(1e+19);                 // «10000000000000000000»
			 *          fmk.noexp(1e-5);                  // «0.00001»
			 *          fmk.noexp(1536. / 1024.);         // «1.5»
			 *          fmk.noexp(0.111);                 // «0.111»
			 *          fmk.noexp(-2986.808299);          // «-2986.808299»
			 *          @endcode
			 *
			 * @note Запись не зависит от установленной локали: разделителем дробной части
			 *       всегда служит точка, разделителей разрядов запись не содержит
			 *
			 * @note Округления запись не выполняет: число, требующее семнадцати значащих
			 *       разрядов, все семнадцать и получит. Для краткой записи с потерей
			 *       точности следует пользоваться перегрузкой с размером шага
			 *
			 * @note Довод вывода одних лишь разрядов сохранён ради совместимости вызовов
			 *       и на запись не влияет: посторонних символов она не содержит
			 *
			 * @param number  число для перевода
			 * @param onlyNum выводить только числа
			 * @return        число в безэкспоненциальной форме
			 *
			 * @see noexp(const double, const uint8_t)
			 *
			 */
			string noexp(const double number, const bool onlyNum = false) const noexcept;
		public:
			/**
			 * @brief Метод порверки на сколько процентов (A > B) или (A < B)
			 *
			 * @param a первое число
			 * @param b второе число
			 * @return  результат расчёта
			 *
			 */
			float rate(const float a, const float b) const noexcept;
			/**
			 * @brief Метод приведения количества символов после запятой к указанному количества
			 *
			 * @param x число для приведения
			 * @param n количество символов после запятой
			 * @return  сформированное число
			 *
			 */
			double floor(const double x, const uint8_t n) const noexcept;
		public:
			/**
			 * @brief Метод перевода римских цифр в арабские
			 *
			 * @param word римское число
			 * @return     арабское число
			 *
			 */
			uint16_t rome2arabic(string_view word) const noexcept;
			/**
			 * @brief Метод перевода римских цифр в арабские
			 *
			 * @param word римское число
			 * @return     арабское число
			 *
			 */
			uint16_t rome2arabic(wstring_view word) const noexcept;
		public:
			/**
			 * @brief Метод перевода арабских чисел в римские
			 *
			 * @param number арабское число от 1 до 4999
			 * @return       римское число
			 *
			 */
			wstring arabic2rome(const uint32_t number) const noexcept;
			/**
			 * @brief Метод перевода арабских чисел в римские
			 *
			 * @param word арабское число от 1 до 4999
			 * @return     римское число
			 *
			 */
			string arabic2rome(string_view word) const noexcept;
			/**
			 * @brief Метод перевода арабских чисел в римские
			 *
			 * @param word арабское число от 1 до 4999
			 * @return     римское число
			 *
			 */
			wstring arabic2rome(wstring_view word) const noexcept;
		public:
			/**
			 * @brief Метод подсчёта количества указанной буквы в слове
			 *
			 * @param word   слово в котором нужно подсчитать букву
			 * @param letter букву которую нужно подсчитать
			 * @return       результат подсчёта
			 *
			 */
			size_t countLetter(string_view word, const wchar_t letter) const noexcept;
			/**
			 * @brief Метод подсчёта количества указанной буквы в слове
			 *
			 * @param word   слово в котором нужно подсчитать букву
			 * @param letter букву которую нужно подсчитать
			 * @return       результат подсчёта
			 *
			 */
			size_t countLetter(wstring_view word, const wchar_t letter) const noexcept;
		public:
			/**
			 * @brief Шаблон функции проверки установлен ли бит в указанной позиции
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод проверки установлен ли бит в указанной позиции
			 *
			 * @param pos позиция для проверки
			 * @param num число в бинарном виде для проверки бита
			 * @return    результат проверки
			 *
			 */
			bool isBit(const T pos, const T num) const noexcept;
			/**
			 * @brief Шаблон функции инверсии бита в указанной позиции
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод инверсии бита в указанной позиции
			 *
			 * @param pos позиция для инверсии
			 * @param num число в бинарном виде для инверсии бита
			 * @return    итоговое значение числа после инверсии
			 *
			 */
			T flipBit(const T pos, const T num) const noexcept;
			/**
			 * @brief Шаблон функции сброса бита в указанной позиции
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод сброса бита в указанной позиции
			 *
			 * @param pos позиция для сброса
			 * @param num число в бинарном виде для сброса бита
			 * @return    итоговое значение числа после сброса бита
			 *
			 */
			T resetBit(const T pos, const T num) const noexcept;
			/**
			 * @brief Шаблон функции устанвки бита в указанную позицию
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод устанвки бита в указанную позицию
			 *
			 * @param pos позиция для установки бита
			 * @param num начальное значение бита
			 * @return    итоговое значение числа после установки бита
			 *
			 */
			T setBit(const T pos, const T num = 0) const noexcept;
		public:
			/**
			 * @brief Метод реализации функции формирования форматированной строки
			 *
			 * @param format формат строки вывода
			 * @param args   передаваемые аргументы
			 * @return       сформированная строка
			 *
			 */
			string format(const char * format, ...) const noexcept;
			/**
			 * @brief Метод реализации функции формирования форматированной строки
			 *
			 * @param format формат строки вывода
			 * @param args   передаваемые аргументы
			 * @return       сформированная строка
			 *
			 */
			wstring format(const wchar_t * format, ...) const noexcept;
		public:
			/**
			 * @brief Метод реализации функции формирования форматированной строки
			 *
			 * @param format формат строки вывода
			 * @param items  список аргументов строки
			 * @return       сформированная строка
			 *
			 */
			string format(string_view format, const vector <string> & items) const noexcept;
			/**
			 * @brief Метод реализации функции формирования форматированной строки
			 *
			 * @param format формат строки вывода
			 * @param items  список аргументов строки
			 * @return       сформированная строка
			 *
			 */
			wstring format(wstring_view format, const vector <wstring> & items) const noexcept;
		public:
			/**
			 * @brief Метод проверки существования слова в тексте
			 *
			 * @param word слово для проверки
			 * @param text текст в котором выполнения проверка
			 * @return     результат выполнения проверки
			 *
			 */
			bool exists(string_view word, string_view text) const noexcept;
			/**
			 * @brief Метод проверки существования слова в тексте
			 *
			 * @param word слово для проверки
			 * @param text текст в котором выполнения проверка
			 * @return     результат выполнения проверки
			 *
			 */
			bool exists(wstring_view word, wstring_view text) const noexcept;
		public:
			/**
			 * @brief Метод замены в тексте слово на другое слово
			 *
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 *
			 */
			string & replace(string & text, const string & word, const string & alt = "") const noexcept;
			/**
			 * @brief Метод замены в тексте слово на другое слово
			 *
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 *
			 */
			wstring & replace(wstring & text, const wstring & word, const wstring & alt = L"") const noexcept;
		public:
			/**
			 * @brief Метод замены в тексте слово на другое слово
			 *
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 *
			 */
			const string & replace(const string & text, const string & word, const string & alt = "") const noexcept;
			/**
			 * @brief Метод замены в тексте слово на другое слово
			 *
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 *
			 */
			const wstring & replace(const wstring & text, const wstring & word, const wstring & alt = L"") const noexcept;
		private:
			/**
			 * @brief Метод извлечения списка символов экранирования по умолчанию
			 *
			 * @return список символов экранирования по умолчанию
			 *
			 */
			static const vector <string> & escapingText() noexcept;
			/**
			 * @brief Метод извлечения списка символов экранирования по умолчанию
			 *
			 * @return список символов экранирования по умолчанию
			 *
			 */
			static const vector <wstring> & escapingWide() noexcept;
		public:
			/**
			 * @brief Намеренные решения разбора записей ключ-значение
			 *
			 * @details Разбор рассчитан на форматы вида CEF, где значение записи не отделено от следующего ключа
			 *          ничем, кроме разделителя записей, поэтому приняты следующие решения:
			 *
			 *          1. Значение может состоять из нескольких слов и содержать разделитель записей: концом
			 *             значения считается последний разделитель, встреченный перед разделителем ключа и
			 *             значения следующей записи (rt=Feb 17 2023 23:30:15.734 YEKT smac=...).
			 *          2. Если следующей записи нет, значение занимает весь остаток текста, а не обрывается на
			 *             первом разделителе, иначе последняя запись вела бы себя иначе, чем все остальные.
			 *          3. Пустое значение является полноценной записью и возвращается с пустой строкой: в CEF
			 *             запись вида cs3= осмысленна и образует пару с cs3Label=CVEID.
			 *          4. Разделители записей в начале текста и между записями пропускаются.
			 *          5. Символ считается экранированным при нечётном количестве предшествующих обратных слэшей,
			 *             что позволяет разбирать значения вида originsicname=CN\=chr-cpsg-01,O\=stal.
			 *          6. Повторяющиеся ключи сохраняются полностью, поэтому результатом является multimap:
			 *             в реальных журналах ключ повторяется (ad.prog-id, deviceExternalId в auditd).
			 *          7. Относительный порядок одинаковых ключей в контейнере зависит от реализации: если важен
			 *             порядок следования записей в исходном тексте, следует использовать потоковый разбор,
			 *             который отдаёт записи строго в порядке их появления.
			 *
			 */
			/**
			 * @brief Метод извлечения ключей и значений из текста
			 *
			 * @param text      текст из которого извлекаются записи
			 * @param delim     разделитель записей
			 * @param separator разделитель ключа и значения
			 * @param escaping  символы экранирования
			 * @return          список найденных элементов
			 *
			 */
			unordered_multimap <string, string> kv(string_view text, string_view delim, string_view separator = "=", const vector <string> & escaping = escapingText()) const noexcept;
			/**
			 * @brief Метод извлечения ключей и значений из текста
			 *
			 * @param text      текст из которого извлекаются записи
			 * @param delim     разделитель записей
			 * @param separator разделитель ключа и значения
			 * @param escaping  символы экранирования
			 * @return          список найденных элементов
			 *
			 */
			unordered_multimap <wstring, wstring> kv(wstring_view text, wstring_view delim, wstring_view separator = L"=", const vector <wstring> & escaping = escapingWide()) const noexcept;
		public:
			/**
			 * @brief Метод потокового извлечения ключей и значений из текста
			 *
			 * @param sid       идентификатор потока разбора
			 * @param text      текст из которого извлекаются записи
			 * @param delim     разделитель записей
			 * @param callback  функция обратного вызова для каждой найденной записи
			 * @param separator разделитель ключа и значения
			 * @param escaping  символы экранирования
			 *
			 */
			void kv(const uint64_t sid, string_view text, string_view delim, function <void (const uint64_t, const string_view, const string_view)> callback, string_view separator = "=", const vector <string> & escaping = escapingText()) const noexcept;
			/**
			 * @brief Метод потокового извлечения ключей и значений из текста
			 *
			 * @param sid       идентификатор потока разбора
			 * @param text      текст из которого извлекаются записи
			 * @param delim     разделитель записей
			 * @param callback  функция обратного вызова для каждой найденной записи
			 * @param separator разделитель ключа и значения
			 * @param escaping  символы экранирования
			 *
			 */
			void kv(const uint64_t sid, wstring_view text, wstring_view delim, function <void (const uint64_t, const wstring_view, const wstring_view)> callback, wstring_view separator = L"=", const vector <wstring> & escaping = escapingWide()) const noexcept;
		public:
			/**
			 * @brief Метод установки пользовательской зоны
			 *
			 * @param zone пользовательская зона
			 *
			 */
			void domainZone(const string_view zone) noexcept;
			/**
			 * @brief Метод установки списка пользовательских зон
			 *
			 * @param zones список доменных зон интернета
			 *
			 */
			void domainZones(const unordered_set <string> & zones) noexcept;
			/**
			 * @brief Метод извлечения списка пользовательских зон интернета
			 *
			 * @return список доменных зон
			 *
			 */
			const unordered_set <string> & domainZones() const noexcept;
		public:
			/**
			 * @brief Метод установки системной локали
			 *
			 * @param locale локализация приложения
			 *
			 */
			void setLocale(string_view locale = AWH_LOCALE) noexcept;
		public:
			/**
			 * @brief Метод извлечения координат url адресов в строке
			 *
			 * @param text текст для извлечения url адресов
			 * @return     список координат с url адресами
			 *
			 */
			unordered_map <size_t, size_t> urls(string_view text) const noexcept;
		public:
			/**
			 * @brief Метод получения иконки
			 *
			 * @param end флаг завершения работы
			 * @return    иконка напутствия работы
			 *
			 */
			string icon(const bool end = false) const noexcept;
		public:
			/**
			 * @brief Метод получения размера в байтах из строки
			 *
			 * @param str строка обозначения размерности (b, Kb, Mb, Gb, Tb)
			 * @return    размер в байтах
			 *
			 */
			double bytes(const string_view str) const noexcept;
			/**
			 * @brief Метод конвертации байт в строку
			 *
			 * @param value   количество байт
			 * @param onlyNum выводить только числа
			 * @return        полученная строка
			 *
			 */
			string bytes(const double value, const bool onlyNum = false) const noexcept;
		public:
			/**
			 * @brief Метод получения количества байт в секунду из строки
			 *
			 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
			 * @return    количество байт в секунду
			 *
			 */
			size_t bpsSize(const string_view str) const noexcept;
			/**
			 * @brief Метод получения размера буфера в байтах
			 *
			 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
			 * @return    размер буфера в байтах
			 *
			 */
			size_t bpsBuffer(const string_view str) const noexcept;
		public:
			/**
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
			 *
			 */
			void setLogger(const Logging * log) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Framework() noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param locale локализация приложения
			 *
			 */
			explicit Framework(string_view locale) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Framework() noexcept {}
	} fmk_t;
};

#endif // __AWH_FRAMEWORK__
