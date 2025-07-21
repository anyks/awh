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
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_REGEXP__
#define __AWH_REGEXP__

/**
 * Стандартные модули
 */
#include <map>
#include <mutex>
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

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * RegExp Класс объекта регулярных выражения
	 */
	typedef class AWHSHARED_EXPORT RegExp {
		private:
			/**
			 * Mutex структура рабочих мютексов
			 */
			typedef struct Mutex {
				// Мютекс контроля матчинга
				std::mutex match;
				// Мютекс контроля записи в кэш
				std::mutex cache;
			} mtx_t;
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
			class AWHSHARED_EXPORT Expression {
				private:
					// Флаг инициализации
					bool _mode;
				public:
					// Объект контекста регулярного выражения
					regex_t reg;
				public:
					/**
					 * Оператор проверки на инициализацию регулярного выражения
					 * @return результат проверки
					 */
					operator bool() const noexcept;
				public:
					/**
					 * operator Оператор установки флага инициализации
					 * @param mode флаг инициализации для установки
					 * @return     текущий объект регулярного выражения
					 */
					Expression & operator = (const bool mode) noexcept;
				public:
					/**
					 * Expression Конструктор
					 */
					Expression() noexcept;
					/**
					 * ~Expression Деструктор
					 */
					~Expression() noexcept;
			};
		public:
			// Создаём новый тип данных регулярного выражения
			using exp_t = std::shared_ptr <Expression>;
			// Создаём новый тип данных для статического хранения регулярных выражений
			using exp_weak_t = std::weak_ptr <Expression>;
		private:
			// Текст ошибки
			string _error;
		private:
			// Мютексы для блокировки потоков
			mutable mtx_t _mtx;
		public:
			// Кэш собранных регулярных выражений
			mutable std::map <pair <int32_t, string>, exp_weak_t> _cache;
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
			bool test(const string & text, const exp_t & exp) const noexcept;
			/**
			 * test Метод проверки регулярного выражения
			 * @param text текст для обработки
			 * @param size размер текста для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат проверки регулярного выражения
			 */
			bool test(const char * text, const size_t size, const exp_t & exp) const noexcept;
		public:
			/**
			 * exec Метод запуска регулярного выражения
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <string> exec(const string & text, const exp_t & exp) const noexcept;
			/**
			 * exec Метод запуска регулярного выражения
			 * @param text текст для обработки
			 * @param size размер текста для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <string> exec(const char * text, const size_t size, const exp_t & exp) const noexcept;
		public:
			/**
			 * match Метод выполнения регулярного выражения
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <pair <size_t, size_t>> match(const string & text, const exp_t & exp) const noexcept;
			/**
			 * match Метод выполнения регулярного выражения
			 * @param text текст для обработки
			 * @param size размер текста для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <pair <size_t, size_t>> match(const char * text, const size_t size, const exp_t & exp) const noexcept;
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
			~RegExp() noexcept {}
	} regexp_t;
};

#endif // __AWH_REGEXP__
