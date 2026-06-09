/**
 * @file: reg.hpp
 * @date: 2025-10-25
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEXP__
#define __AWH_REGEXP__

/**
 * Стандартные модули
 */
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <cstdint>

/**
 * Наши модули
 */
#include "global.hpp"
#include "locker.hpp"

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
	 * @brief Класс объекта регулярных выражения
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Regular_Expressions {
		private:
			/**
			 * @brief структура рабочих мютексов
			 *
			 */
			typedef struct Mutex {
				// Мютекс контроля матчинга
				lock_state_t <std::mutex> match;
				// Мютекс контроля записи в кэш
				lock_state_t <std::mutex> cache;
			} mtx_t;
		public:
			/**
			 * @brief Опции работы с регулярными выражениями
			 *
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
			 * @brief Класс регулярного выражения
			 *
			 */
			class __AWH_SHARED_EXPORT__ Expression;
		public:
			/**
			 * Создаём новый тип данных регулярного выражения
			 */
			using exp_t = std::shared_ptr <Expression>;
			/**
			 * Создаём новый тип данных для статического хранения регулярных выражений
			 */
			using exp_weak_t = std::weak_ptr <Expression>;
		private:
			// Текст ошибки
			string _error;
		private:
			// Мютексы для блокировки потоков
			mutable mtx_t _mtx;
		private:
			// Кэш собранных регулярных выражений
			mutable std::map <std::pair <int32_t, string>, exp_weak_t> _cache;
		private:
			// Объект логирования
			const Logging * _log;
		public:
			/**
			 * @brief Метод извлечения текста ошибки регулярного выражения
			 *
			 * @return текст ошибки регулярного выражения
			 */
			const string & error() const noexcept;
		public:
			/**
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode флаг режима безопасности потоков
			 */
			void threadSafety(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод проверки регулярного выражения
			 *
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат проверки регулярного выражения
			 */
			bool test(string_view text, const exp_t & exp) const noexcept;
			/**
			 * @brief Метод проверки регулярного выражения
			 *
			 * @param text текст для обработки
			 * @param size размер текста для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат проверки регулярного выражения
			 */
			bool test(const char * text, const size_t size, const exp_t & exp) const noexcept;
		public:
			/**
			 * @brief Метод запуска регулярного выражения
			 *
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <string> exec(string_view text, const exp_t & exp) const noexcept;
			/**
			 * @brief Метод запуска регулярного выражения
			 *
			 * @param text текст для обработки
			 * @param size размер текста для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <string> exec(const char * text, const size_t size, const exp_t & exp) const noexcept;
		public:
			/**
			 * @brief Метод выполнения регулярного выражения
			 *
			 * @param text текст для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <std::pair <size_t, size_t>> match(string_view text, const exp_t & exp) const noexcept;
			/**
			 * @brief Метод выполнения регулярного выражения
			 *
			 * @param text текст для обработки
			 * @param size размер текста для обработки
			 * @param exp  объект регулярного выражения
			 * @return     результат обработки регулярного выражения
			 */
			vector <std::pair <size_t, size_t>> match(const char * text, const size_t size, const exp_t & exp) const noexcept;
		public:
			/**
			 * @brief Метод сборки регулярного выражения
			 *
			 * @param pattern регулярное выражение для сборки
			 * @param options список опций для сборки регулярного выражения
			 * @return        результат собранного регулярного выражения
			 */
			exp_t build(string_view pattern, const vector <option_t> & options = {}) const noexcept;
		public:
			/**
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
			 */
			void setLogger(const Logging * log) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Regular_Expressions() noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param log объект работы с логами
			 */
			explicit Regular_Expressions(const Logging * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Regular_Expressions() noexcept;
	} regexp_t;
};

#endif // __AWH_REGEXP__
