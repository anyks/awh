/**
 * @file: nwt.hpp
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

#ifndef __AWH_NWT__
#define __AWH_NWT__

/**
 * Стандартные модули
 */
#include <cinttypes>
#include <unordered_set>

/**
 * Наши модули
 */
#include "../sys/lib.hpp"
#include "../sys/reg.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Прототип класса работы с логами
	 *
	 */
	class Log;
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Структура списка параметров URL
	 *
	 */
	typedef class AWH_SHARED_EXPORT NWT {
		public:
			// Типы URL-адреса
			enum class types_t : uint8_t {
				NONE  = 0x00, // Тип не определён
				MAC   = 0x01, // MAC-адрес
				URL   = 0x02, // URL-адрес
				IPV4  = 0x03, // IPv4-адрес
				IPV6  = 0x04, // IPv6-адрес
				EMAIL = 0x05  // Электронная почта
			};
		public:
			/**
			 * @brief Класс URL-адреса
			 *
			 */
			typedef class AWH_SHARED_EXPORT URL {
				public:
					types_t type;  // Тип URL-адреса
					uint32_t port; // Порт URL-адреса
					string uri;    // Полный URI-параметры
					string host;   // Хост URL-адреса
					string path;   // Путь URL-адреса
					string user;   // Ник пользователя (для электронной почты)
					string pass;   // Пароль пользователя
					string anchor; // Якорь URL-адреса
					string domain; // Домен верхнего уровня
					string params; // Параметры URL-адреса
					string schema; // Протокол URL-адреса
				public:
					/**
					 * @brief Оператор перемещения
					 *
					 * @param url параметры падреса
					 * @return    параметры URL-запроса
					 */
					URL & operator = (URL && url) noexcept;
					/**
					 * @brief Оператор присванивания
					 *
					 * @param url параметры падреса
					 * @return    параметры URL-запроса
					 */
					URL & operator = (const URL & url) noexcept;
				public:
					/**
					 * @brief Оператор сравнения
					 *
					 * @param url параметры падреса
					 * @return    результат сравнения
					 */
					bool operator == (const URL & url) noexcept;
				public:
					/**
					 * @brief Конструктор перемещения
					 *
					 * @param url параметры падреса
					 */
					URL(URL && url) noexcept;
					/**
					 * @brief Конструктор копирования
					 *
					 * @param url параметры падреса
					 */
					URL(const URL & url) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					URL() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~URL() noexcept {}
			} url_t;
		private:
			// Список букв разрешенных в последовательности
			string _letters;
		private:
			// Объект регулярного выражения
			regexp_t _regexp;
		private:
			// Регулярное выражение IP адресов
			regexp_t::exp_t _ip;
			// Регулярное выражение URL адресов
			regexp_t::exp_t _url;
			// Регулярное выражение E-MAIL адресов
			regexp_t::exp_t _email;
		private:
			// Список пользовательских доменных зон интернета
			std::unordered_set <string> _user;
			// Список основных доменных зон интернета
			std::unordered_set <string> _general;
			// Список интернациональных доменных зон интернета
			std::unordered_set <string> _national;
		private:
			// Объект логера
			const Log * _log;
		private:
			/**
			 * @brief Метод инициализации
			 *
			 */
			void init() noexcept;
		public:
			/**
			 * @brief Метод установки пользовательской зоны
			 *
			 * @param zone пользовательская зона
			 */
			void zone(const string & zone) noexcept;
		public:
			/**
			 * @brief Метод извлечения списка пользовательских зон интернета
			 *
			 */
			const std::unordered_set <string> & zones() const noexcept;
			/**
			 * @brief Метод установки списка пользовательских зон
			 *
			 * @param zones список доменных зон интернета
			 */
			void zones(const std::unordered_set <string> & zones) noexcept;
		public:
			/**
			 * @brief Метод парсинга URI-строки
			 *
			 * @param text текст для парсинга
			 * @return     параметры полученные в результате парсинга
			 */
			url_t parse(const string & text) noexcept;
		public:
			/**
			 * @brief Метод очистки результатов парсинга
			 *
			 */
			void clear() noexcept;
			/**
			 * @brief Метод добавления букв алфавита
			 *
			 * @param letters список букв алфавита
			 */
			void letters(const string & letters = "") noexcept;
		public:
			/**
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
			 */
			void setLogger(const Log * log) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit NWT() noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 */
			explicit NWT(const Log * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param letters список букв алфавита
			 * @param log     объект для работы с логами
			 */
			explicit NWT(const string & letters, const Log * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~NWT() noexcept {}
	} nwt_t;
};

#endif // __AWH_NWT__
