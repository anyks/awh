/**
 * @file: nwt.hpp
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

#ifndef __AWH_NWT__
#define __AWH_NWT__

/**
 * Стандартная библиотека
 */
#include <set>
#include <sys/types.h>

/**
 * Наши модули
 */
#include <sys/win.hpp>
#include <sys/lib.hpp>
#include <sys/reg.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * NWT Структура списка параметров URI
	 */
	typedef class NWT {
		public:
			// Типы n-грамм
			enum class types_t : uint8_t {
				MAC     = 0x00, // MAC адрес
				URL     = 0x01, // URL адрес
				IPV4    = 0x02, // IPv4 адрес
				IPV6    = 0x03, // IPv6 адрес
				NONE    = 0x04, // Тип не определён
				EMAIL   = 0x05, // Электронная почта
				WRONG   = 0x06, // Сломанный адрес
				NETWORK = 0x07  // Параметры сети
			};
		public:
			/**
			 * Uri Структура ответа
			 */
			typedef struct Uri {
				u_int port;    // Порт запроса если существует
				types_t type;  // Тип URI
				string uri;    // Полный URI
				string host;   // Хост URL адреса
				string path;   // Путь запроса если существует
				string user;   // Ник пользователя (для электронной почты)
				string pass;   // Пароль пользователя
				string anchor; // Якорь URI параметров запроса
				string domain; // Домен верхнего уровня если существует
				string params; // Параметры запроса если существуют
				string schema; // Протокол запроса если существует
				/**
				 * Uri Конструктор
				 */
				Uri() noexcept :
				 port(0), type(types_t::NONE), uri{""},
				 host{""}, path{""}, user{""}, pass{""},
				 anchor{""}, domain{""}, params{""}, schema{""} {}
			} uri_t;
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
			set <string> _user;
			// Список основных доменных зон интернета
			set <string> _general;
			// Список интернациональных доменных зон интернета
			set <string> _national;
		public:
			/**
			 * zone Метод установки пользовательской зоны
			 * @param zone пользовательская зона
			 */
			void zone(const string & zone) noexcept;
		public:
			/**
			 * zones Метод извлечения списка пользовательских зон интернета
			 */
			const set <string> & zones() const noexcept;
			/**
			 * zones Метод установки списка пользовательских зон
			 * @param zones список доменных зон интернета
			 */
			void zones(const set <string> & zones) noexcept;
		public:
			/**
			 * parse Метод парсинга URI строки
			 * @param text текст для парсинга
			 * @return     параметры полученные в результате парсинга
			 */
			uri_t parse(const string & text) noexcept;
		public:
			/**
			 * clear Метод очистки результатов парсинга
			 */
			void clear() noexcept;
			/**
			 * letters Метод добавления букв алфавита
			 * @param letters список букв алфавита
			 */
			void letters(const string & letters) noexcept;
		public:
			/**
			 * NWT Конструктор
			 * @param letters список букв алфавита
			 */
			NWT(const string & letters = "") noexcept;
			/**
			 * ~NWT Деструктор
			 */
			~NWT() noexcept;
	} nwt_t;
};

#endif // __AWH_NWT__
