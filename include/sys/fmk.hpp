/**
 * @file: fmk.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_FRAMEWORK__
#define __AWH_FRAMEWORK__

/**
 * Стандартная библиотека
 */
#include <set>
#include <map>
#include <list>
#include <ctime>
#include <cmath>
#include <chrono>
#include <locale>
#include <string>
#include <vector>
#include <limits>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <type_traits>
#include <sys/types.h>

// Если используется BOOST
#ifdef USE_BOOST_CONVERT
	#include <boost/locale/encoding_utf.hpp>
// Если нужно использовать стандартную библиотеку
#else
	#include <codecvt>
#endif

/**
 * Наши модули
 */
#include <sys/win.hpp>
#include <net/nwt.hpp>

/**
 * Подключаем OpenSSL
 */
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Framework Класс фреймворка
	 */
	typedef class Framework {
		private:
			/**
			 * Nums структура параметров чисел
			 */
			typedef struct Nums {
				// Названия римских цифр
				const set <wchar_t> roman = {L'm', L'd', L'c', L'l', L'x', L'i', L'v'};
				// Список арабских цифр
				const set <wchar_t> arabs = {L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9'};
				// Шаблоны римских форматов
				const vector <wstring> i = {L"", L"I", L"II", L"III", L"IV", L"V", L"VI", L"VII", L"VIII", L"IX"};
				const vector <wstring> x = {L"", L"X", L"XX", L"XXX", L"XL", L"L", L"LX", L"LXX", L"LXXX", L"XC"};
				const vector <wstring> c = {L"", L"C", L"CC", L"CCC", L"CD", L"D", L"DC", L"DCC", L"DCCC", L"CM"};
				const vector <wstring> m = {L"", L"M", L"MM", L"MMM", L"MMMM"};
			} nums_t;
		private:
			// Объект парсинга nwt адреса
			mutable nwt_t nwt;
			// Числовые параметры
			const nums_t numsSymbols;
			// Латинский алфавит
			std::set <wchar_t> latian = {
				L'a', L'b', L'c', L'd', L'e',
				L'f', L'g', L'h', L'i', L'j',
				L'k', L'l', L'm', L'n', L'o',
				L'p', L'q', L'r', L's', L't',
				L'u', L'v', L'w', L'x', L'y', L'z'
			};
			// Устанавливаем локаль по умолчанию
			std::locale locale{AWH_LOCALE};
		public:
			/**
			 * os_t Названия поддерживаемых операционных систем
			 */
			enum class os_t : uint8_t {NONE, LINUX, MACOSX, FREEBSD, UNIX, WIND32, WIND64};
		public:
			/**
			 * trim Метод удаления пробелов вначале и конце текста
			 * @param text текст для удаления пробелов
			 * @return     результат работы функции
			 */
			string trim(const string & text) const noexcept;
			/**
			 * convert Метод конвертирования строки utf-8 в строку
			 * @param str строка utf-8 для конвертирования
			 * @return    обычная строка
			 */
			string convert(const wstring & str) const noexcept;
		public:
			/**
			 * toLower Метод перевода русских букв в нижний регистр
			 * @param str строка для перевода
			 * @return    строка в нижнем регистре
			 */
			string toLower(const string & str) const noexcept;
			/**
			 * toUpper Метод перевода русских букв в верхний регистр
			 * @param str строка для перевода
			 * @return    строка в верхнем регистре
			 */
			string toUpper(const string & str) const noexcept;
			/**
			 * smartUpper Метод умного перевода символов в верхний регистр
			 * @param str строка для перевода
			 * @return    строка в верхнем регистре
			 */
			string smartUpper(const string & str) const noexcept;
		public:
			/**
			 * decToHex Метод конвертации 10-го числа в 16-е
			 * @param number число для конвертации
			 * @return       результат конвертации
			 */
			string decToHex(const size_t number) const noexcept;
			/**
			 * hexToDec Метод конвертации 16-го числа в 10-е
			 * @param number число для конвертации
			 * @return       результат конвертации
			 */
			size_t hexToDec(const string & number) const noexcept;
		public:
			/**
			 * floorN Метод приведения количества символов после запятой к указанному количества
			 * @param x число для приведения
			 * @param n количество символов после запятой
			 * @return  сформированное число
			 */
			double floorN(const double x, const uint8_t n) const noexcept;
		public:
			/**
			 * noexp Метод перевода числа в безэкспоненциальную форму
			 * @param number число для перевода
			 * @param step   размер шага после запятой
			 * @return       число в безэкспоненциальной форме
			 */
			string noexp(const double number, const double step) const noexcept;
			/**
			 * noexp Метод перевода числа в безэкспоненциальную форму
			 * @param number  число для перевода
			 * @param onlyNum выводить только числа
			 * @return        число в безэкспоненциальной форме
			 */
			string noexp(const double number, const bool onlyNum = false) const noexcept;
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
			 * @param  format формат строки вывода
			 * @param  items  список аргументов строки
			 * @return        сформированная строка
			 */
			string format(const string & format, const vector <string> & items) const noexcept;
		public:
			/**
			 * itoa Метод конвертации чисел в указанную систему счисления
			 * @param value число для конвертации
			 * @param radix система счисления
			 * @return      полученная строка в системе счисления
			 */
			string itoa(const int64_t value, const uint8_t radix) const noexcept;
			/**
			 * atoi Метод конвертации строковых чисел в десятичную систему счисления
			 * @param value число для конвертации
			 * @param radix система счисления
			 * @return      полученная строка в системе счисления
			 */
			int64_t atoi(const string & value, const uint8_t radix) const noexcept;
		public:
			/**
			 * toLower Метод перевода русских букв в нижний регистр
			 * @param str строка для перевода
			 * @return    строка в нижнем регистре
			 */
			char toLower(const char letter) const noexcept;
			/**
			 * toUpper Метод перевода русских букв в верхний регистр
			 * @param str строка для перевода
			 * @return    строка в верхнем регистре
			 */
			char toUpper(const char letter) const noexcept;
		public:
			/**
			 * toLower Метод перевода русских букв в нижний регистр
			 * @param str строка для перевода
			 * @return    строка в нижнем регистре
			 */
			wchar_t toLower(const wchar_t letter) const noexcept;
			/**
			 * toUpper Метод перевода русских букв в верхний регистр
			 * @param str строка для перевода
			 * @return    строка в верхнем регистре
			 */
			wchar_t toUpper(const wchar_t letter) const noexcept;
		public:
			/**
			 * trim Метод удаления пробелов вначале и конце текста
			 * @param text текст для удаления пробелов
			 * @return     результат работы функции
			 */
			wstring trim(const wstring & text) const noexcept;
			/**
			 * convert Метод конвертирования строки в строку utf-8
			 * @param str строка для конвертирования
			 * @return    строка в utf-8
			 */
			wstring convert(const string & str) const noexcept;
		public:
			/**
			 * toLower Метод перевода русских букв в нижний регистр
			 * @param str строка для перевода
			 * @return    строка в нижнем регистре
			 */
			wstring toLower(const wstring & str) const noexcept;
			/**
			 * toUpper Метод перевода русских букв в верхний регистр
			 * @param str строка для перевода
			 * @return    строка в верхнем регистре
			 */
			wstring toUpper(const wstring & str) const noexcept;
			/**
			 * smartUpper Метод умного перевода символов в верхний регистр
			 * @param str строка для перевода
			 * @return    строка в верхнем регистре
			 */
			wstring smartUpper(const wstring & str) const noexcept;
		public:
			/**
			 * arabic2Roman Метод перевода арабских чисел в римские
			 * @param number арабское число от 1 до 4999
			 * @return       римское число
			 */
			wstring arabic2Roman(const u_int number) const noexcept;
			/**
			 * arabic2Roman Метод перевода арабских чисел в римские
			 * @param word арабское число от 1 до 4999
			 * @return     римское число
			 */
			wstring arabic2Roman(const wstring & word) const noexcept;
		public:
			/**
			 * replace Метод замены в тексте слово на другое слово
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 */
			wstring replace(const wstring & text, const wstring & word, const wstring & alt = L"") const noexcept;
		public:
			/**
			 * roman2Arabic Метод перевода римских цифр в арабские
			 * @param word римское число
			 * @return     арабское число
			 */
			u_short roman2Arabic(const wstring & word) const noexcept;
		public:
			/**
			 * setCase Метод запоминания регистра слова
			 * @param pos позиция для установки регистра
			 * @param cur текущее значение регистра в бинарном виде
			 * @return    позиция верхнего регистра в бинарном виде
			 */
			size_t setCase(const size_t pos, const size_t cur = 0) const noexcept;
			/**
			 * countLetter Метод подсчета количества указанной буквы в слове
			 * @param word   слово в котором нужно подсчитать букву
			 * @param letter букву которую нужно подсчитать
			 * @return       результат подсчёта
			 */
			size_t countLetter(const wstring & word, const wchar_t letter) const noexcept;
		public:
			/**
			 * isUrl Метод проверки соответствия слова url адресу
			 * @param word слово для проверки
			 * @return     результат проверки
			 */
			bool isUrl(const wstring & word) const noexcept;
			/**
			 * isUpper Метод проверки символ на верхний регистр
			 * @param letter буква для проверки
			 * @return       результат проверки
			 */
			bool isUpper(const wchar_t letter) const noexcept;
			/**
			 * isSpace Метод проверки является ли буква, пробелом
			 * @param letter буква для проверки
			 * @return       результат проверки
			 */
			bool isSpace(const wchar_t letter) const noexcept;
		public:
			/**
			 * isLatian Метод проверки является ли строка латиницей
			 * @param str строка для проверки
			 * @return    результат проверки
			 */
			bool isLatian(const wstring & str) const noexcept;
			/**
			 * checkLatian Метод проверки наличия латинских символов в строке
			 * @param str строка для проверки
			 * @return    результат проверки
			 */
			bool checkLatian(const wstring & str) const noexcept;
		public:
			/**
			 * isNumber Метод проверки является ли слово числом
			 * @param word слово для проверки
			 * @return     результат проверки
			 */
			bool isNumber(const string & word) const noexcept;
			/**
			 * isNumber Метод проверки является ли слово числом
			 * @param word слово для проверки
			 * @return     результат проверки
			 */
			bool isNumber(const wstring & word) const noexcept;
			/**
			 * isANumber Метод проверки является ли косвенно слово числом
			 * @param word слово для проверки
			 * @return     результат проверки
			 */
			bool isANumber(const wstring & word) const noexcept;
		public:
			/**
			 * isDecimal Метод проверки является ли слово дробным числом
			 * @param word слово для проверки
			 * @return     результат проверки
			 */
			bool isDecimal(const string & word) const noexcept;
			/**
			 * isDecimal Метод проверки является ли слово дробным числом
			 * @param word слово для проверки
			 * @return     результат проверки
			 */
			bool isDecimal(const wstring & word) const noexcept;
		public:
			/**
			 * domainZone Метод установки пользовательской зоны
			 * @param zone пользовательская зона
			 */
			void domainZone(const string & zone) noexcept;
			/**
			 * domainZone Метод установки пользовательской зоны
			 * @param zone пользовательская зона
			 */
			void domainZone(const wstring & zone) noexcept;
			/**
			 * domainZones Метод установки списка пользовательских зон
			 * @param zones список доменных зон интернета
			 */
			void domainZones(const std::set <string> & zones) noexcept;
			/**
			 * domainZones Метод установки списка пользовательских зон
			 * @param zones список доменных зон интернета
			 */
			void domainZones(const std::set <wstring> & zones) noexcept;
			/**
			 * domainZones Метод извлечения списка пользовательских зон интернета
			 * @return список доменных зон
			 */
			const std::set <wstring> & domainZones() const noexcept;
		public:
			/**
			 * urls Метод извлечения координат url адресов в строке
			 * @param text текст для извлечения url адресов
			 * @return     список координат с url адресами
			 */
			std::map <size_t, size_t> urls(const wstring & text) const noexcept;
		public:
			/**
			 * split Метод разделения строк на составляющие
			 * @param str   строка для поиска
			 * @param delim разделитель
			 * @param v     результирующий вектор
			 */
			void split(const wstring & str, const wstring & delim, set <wstring> & v) const noexcept;
			/**
			 * split Метод разделения строк на составляющие
			 * @param str   строка для поиска
			 * @param delim разделитель
			 * @param v     результирующий вектор
			 */
			void split(const wstring & str, const wstring & delim, list <wstring> & v) const noexcept;
			/**
			 * split Метод разделения строк на составляющие
			 * @param str   строка для поиска
			 * @param delim разделитель
			 * @param v     результирующий вектор
			 */
			void split(const wstring & str, const wstring & delim, vector <wstring> & v) const noexcept;
		public:
			/**
			 * split Метод разделения строк на составляющие
			 * @param str   строка для поиска
			 * @param delim разделитель
			 * @param v     результирующий вектор
			 */
			void split(const string & str, const string & delim, set <wstring> & v) const noexcept;
			/**
			 * split Метод разделения строк на составляющие
			 * @param str   строка для поиска
			 * @param delim разделитель
			 * @param v     результирующий вектор
			 */
			void split(const string & str, const string & delim, list <wstring> & v) const noexcept;
			/**
			 * split Метод разделения строк на составляющие
			 * @param str   строка для поиска
			 * @param delim разделитель
			 * @param v     результирующий вектор
			 */
			void split(const string & str, const string & delim, vector <wstring> & v) const noexcept;
		public:
			/**
			 * setLocale Метод установки локали
			 * @param locale локализация приложения
			 */
			void setLocale(const string & locale = AWH_LOCALE) noexcept;
		public:
			/**
			 * unixTimestamp Метод получения штампа времени в миллисекундах
			 * @return штамп времени в миллисекундах
			 */
			time_t unixTimestamp() const noexcept;
			/**
			 * nanoTimestamp Метод получения штампа времени в наносекундах
			 * @return штамп времени в наносекундах
			 */
			time_t nanoTimestamp() const noexcept;
		public:
			/**
			 * timeToStr Метод преобразования UnixTimestamp в строку
			 * @param date   дата в UnixTimestamp
			 * @param format формат даты
			 * @return       строка содержащая дату
			 */
			string timeToStr(const time_t date, const string & format = "%a, %d %b %Y %H:%M:%S %Z") const noexcept;
			/**
			 * strToTime Метод перевода строки в UnixTimestamp
			 * @param date   строка даты
			 * @param format формат даты
			 * @return       дата в UnixTimestamp
			 */
			time_t strToTime(const string & date, const string & format = "%a, %d %b %Y %H:%M:%S %Z") const noexcept;
		public:
			/**
			 * timeToAbbr Метод перевода времени в аббревиатуру
			 * @param date дата в UnixTimestamp
			 * @return     строка содержащая аббревиатуру даты
			 */
			string timeToAbbr(const time_t date) const noexcept;
			/**
			 * strpTime Функция получения Unix TimeStamp из строки
			 * @param str    строка с датой
			 * @param format форматы даты
			 * @param tm     объект даты
			 * @return       результат работы
			 */
			string strpTime(const string & str, const string & format, struct tm * tm) const noexcept;
		public:
			/**
			 * os Метод определения операционной системы
			 * @return название операционной системы
			 */
			os_t os() const noexcept;
			/**
			 * icon Метод получения иконки
			 * @param end флаг завершения работы
			 * @return    иконка напутствия работы
			 */
			string icon(const bool end = false) const noexcept;
		public:
			/**
			 * md5 Метод получения md5 хэша из строки
			 * @param text текст для перевода в строку
			 * @return     хэш md5
			 */
			string md5(const string & text) const noexcept;
			/**
			 * sha1 Метод получения sha1 хэша из строки
			 * @param text текст для перевода в строку
			 * @return     хэш sha1
			 */
			string sha1(const string & text) const noexcept;
			/**
			 * sha256 Метод получения sha256 хэша из строки
			 * @param text текст для перевода в строку
			 * @return     хэш sha256
			 */
			string sha256(const string & text) const noexcept;
			/**
			 * sha512 Метод получения sha512 хэша из строки
			 * @param text текст для перевода в строку
			 * @return     хэш sha512
			 */
			string sha512(const string & text) const noexcept;
			/**
			 * hmacsha1 Метод получения подписи hmac sha1
			 * @param key  ключ для подписи
			 * @param text текст для получения подписи
			 * @return     хэш sha1
			 */
			string hmacsha1(const string & key, const string & text) const noexcept;
			/**
			 * hmacsha256 Метод получения подписи hmac sha256
			 * @param key  ключ для подписи
			 * @param text текст для получения подписи
			 * @return     хэш sha256
			 */
			string hmacsha256(const string & key, const string & text) const noexcept;
			/**
			 * hmacsha512 Метод получения подписи hmac sha512
			 * @param key  ключ для подписи
			 * @param text текст для получения подписи
			 * @return     хэш sha512
			 */
			string hmacsha512(const string & key, const string & text) const noexcept;
		public:
			/**
			 * bytes Метод получения размера в байтах из строки
			 * @param str строка обозначения размерности
			 * @return    размер в байтах
			 */
			size_t bytes(const string & str) const noexcept;
			/**
			 * seconds Метод получения размера в секундах из строки
			 * @param str строка обозначения размерности
			 * @return    размер в секундах
			 */
			time_t seconds(const string & str) const noexcept;
		public:
			/**
			 * sizeBuffer Метод получения размера буфера в байтах
			 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
			 * @return    размер буфера в байтах
			 */
			long sizeBuffer(const string & str) const noexcept;
			/**
			 * rate Метод порверки на сколько процентов (A > B) или (A < B)
			 * @param a первое число
			 * @param b второе число
			 * @return  результат расчёта
			 */
			float rate(const float a, const float b) const noexcept;
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
