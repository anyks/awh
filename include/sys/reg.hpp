/**
 * @file: reg.hpp
 * @date: 2023-12-14
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
 * Стандартные модули
 */
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include <pcre2/pcre2posix.h>

/**
 * Разрешаем сборку под Windows
 */
#include <sys/global.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * RegExp Прототип класса работы с регулярными выражениями
	 */
	class RegExp;
	/**
	 * RegExp Класс объекта регулярных выражения
	 */
	typedef class AWHSHARED_EXPORT RegExp {
		public:
			/**
			 * option_t Опции работы с регулярными выражениями
			 */
			enum class option_t : uint8_t {
				NONE      = 0x00, // Не установлено
				UCP       = 0x01, // Поддержка свойств Юникода
				UTF8      = 0x02, // Запускать в режиме UTF-8
				NOSUB     = 0x03, // Не сообщать о том, что было сопоставлено
				DOTALL    = 0x04, // Точка соответствует чему угодно, включая NL
				UNGREEDY  = 0x05, // Инвертировать жадность кванторов
				CASELESS  = 0x06, // Без учёта регистра
				NOTEMPTY  = 0x07, // Блокировка сопоставления пустой строки
				MULTILINE = 0x08  // ^ и $ соответствуют новым строкам в тексте
			};
		private:
			/**
			 * Expression Класс регулярного выражения
			 */
			class Expression {
				private:
					/**
					 * RegExp Устанавливаем дружбу с классом регулярных выражений
					 */
					friend class RegExp;
				private:
					// Флаг инициализации
					bool _mode;
				private:
					// Объект контекста регулярного выражения
					regex_t _reg;
				public:
					/**
					 * Оператор проверки на инициализацию регулярного выражения
					 * @return результат проверки
					 */
					operator bool() const noexcept {
						// Выводим результ проверки инициализации
						return this->_mode;
					}
				public:
					/**
					 * Expression Конструктор
					 */
					Expression() noexcept : _mode(false) {}
					/**
					 * ~Expression Деструктор
					 */
					~Expression() noexcept {}
			};
		public:
			// Создаём новый тип данных регулярного выражения
			typedef std::shared_ptr <Expression> exp_t;
		private:
			// Текст ошибки
			string _error;
		private:
			// Список инициализированных регулярных выражений
			std::map <std::pair <int32_t, string>, regex_t *> _expressions;
		public:
			/**
			 * error Метод извлечения текста ошибки регулярного выражения
			 * @return текст ошибки регулярного выражения
			 */
			const string & error() const noexcept;
		public:
			/**
			 * test Метод проверки регулярного выражения
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат проверки регулярного выражения
			 */
			bool test(const char * text, const exp_t & exp) const noexcept;
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
			vector <string> exec(const char * text, const exp_t & exp) const noexcept;
			/**
			 * exec Метод запуска регулярного выражения
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <string> exec(const string & text, const exp_t & exp) const noexcept;
		public:
			/**
			 * match Метод выполнения регулярного выражения
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <std::pair <size_t, size_t>> match(const char * text, const exp_t & exp) const noexcept;
			/**
			 * match Метод выполнения регулярного выражения
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <std::pair <size_t, size_t>> match(const string & text, const exp_t & exp) const noexcept;
		public:
			/**
			 * build Метод сборки регулярного выражения
			 * @param pattern регулярное выражение для сборки
			 * @param options список опций для сборки регулярного выражения
			 * @return        результат собранного регулярного выражения
			 */
			exp_t build(const string & pattern, const vector <option_t> & options = {}) const noexcept;
		public:
			/**
			 * RegExp Конструктор
			 */
			RegExp() noexcept : _error{""} {}
			/**
			 * ~RegExp Деструктор
			 */
			~RegExp() noexcept;
	} regexp_t;
};

#endif // __AWH_REGEXP__
