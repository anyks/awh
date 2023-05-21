/**
 * @file: re.hpp
 * @date: 2023-05-20
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_REGEXP__
#define __AWH_REGEXP__

/**
 * Стандартная библиотека
 */
#include <string>
#include <vector>
#include <pcre/pcre.h>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * RegExp Класс объекта регулярных выражения
	 */
	typedef class RegExp {
		public:
			/**
			 * Expression Структура регулярного выражения
			 */
			typedef struct Expression {
				// Смещение в тексте позиции ошибки
				int offset;
				// Объект контекста регулярного выражения
				pcre * ctx;
				// Текст сообщения ошибки
				const char * error;
				/**
				 * Expression Конструктор
				 */
				Expression() noexcept : offset(0), ctx(nullptr), error(nullptr) {}
			} exp_t;
		public:
			/**
			 * option_t Опции работы с регулярными выражениями
			 */
			enum class option_t : uint8_t {
				NONE            = 0x00, // Не установлено
				UTF8            = 0x01, // Запускать в режиме UTF-8
				EXTRA           = 0x02, // Дополнительные функции (в настоящее время мало используется)
				DOTALL          = 0x03, // Точка соответствует чему угодно, включая NL
				UNGREEDY        = 0x04, // Инвертировать жадность кванторов
				ANCHORED        = 0x05, // Привязка шаблона якоря
				CASELESS        = 0x06, // Без учёта регистра
				EXTENDED        = 0x07, // Игнорировать пробелы и # комментарии
				MULTILINE       = 0x08, // ^ и $ соответствуют новым строкам в тексте
				BSR_UNICODE     = 0x09, // \R соответствует всем окончаниям строк Unicode
				NEWLINE_CR      = 0x0A, // Установить CR в качестве последовательности новой строки
				NEWLINE_LF      = 0x0B, // Установить LF в качестве последовательности новой строки
				NEWLINE_CRLF    = 0x0C, // Установить CRLF в качестве последовательности новой строки
				BSR_ANYCRLF     = 0x0D, // \R соответствует только CR, LF или CRLF
				AUTO_CALLOUT    = 0x0E, // Скомпилируйте автоматические выноски
				NO_UTF8_CHECK   = 0x0F, // Не проверять шаблон на допустимость UTF-8 (актуально, только если установлен UTF8)
				NO_AUTO_CAPTURE = 0x10  // Отключить пронумерованные скобки захвата (доступны именованные)
			};
		public:
			/**
			 * free Метод очистки объекта регулярного выражения
			 * @param exp объект регулярного выражения
			 */
			void free(const exp_t & exp) const noexcept;
		public:
			/**
			 * test Метод проверки регулярного выражения
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат проверки регулярного выражения
			 */
			bool test(const string & text, const exp_t & exp) const noexcept;
		public:
			/**
			 * exec Метод запуска регулярного выражения
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <string> exec(const string & text, const exp_t & exp) const noexcept;
		public:
			/**
			 * build Метод сборки регулярного выражения
			 * @param expression регулярное выражение для сборки
			 * @param options    список опций для сборки регулярного выражения
			 * @return           результат собранного регулярного выражения
			 */
			exp_t build(const string & expression, const vector <option_t> & options = {}) const noexcept;
		public:
			/**
			 * RegExp Конструктор
			 */
			RegExp() noexcept {}
			/**
			 * ~RegExp Деструктор
			 */
			~RegExp() noexcept {}
	} regexp_t;
};

#endif // __AWH_REGEXP__
