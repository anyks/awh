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
#include <regex>
#include <iostream>
#include <sys/types.h>
/**
 * Наши модули
 */
#include <win.hpp>
#include <lib.hpp>

// Устанавливаем область видимости
using namespace std;

/*
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
				MAC = 0x0,    // MAC адрес
				IPV4 = 0x1,   // IPv4 адрес
				IPV6 = 0x2,   // IPv6 адрес
				NONE = 0x3,   // Тип не определён
				EMAIL = 0x4,  // Электронная почта
				WRONG = 0x5,  // Сломанный адрес
				DHOST = 0x6,  // Доменное имя
				NETWORK = 0x7 // Параметры сети
			};
		public:
			/**
			 * Data Структура ответа
			 */
			typedef struct Data {
				// Основные параметры
				types_t type;     // Тип URI
				wstring uri;      // Полный URI
				wstring port;     // Порт запроса если существует
				wstring data;     // Полное доменное имя, мак адрес, ip адрес или сеть
				wstring path;     // Путь запроса если существует
				wstring user;     // Ник пользователя (для электронной почты)
				wstring domain;   // Домен верхнего уровня если существует
				wstring params;   // Параметры запроса если существуют
				wstring protocol; // Протокол запроса если существует
				/**
				 * Data Конструктор
				 */
				Data() : type(types_t::NONE), uri(L""), data(L""), path(L""), domain(L""), params(L""), protocol(L"") {}
			} data_t;
		private:
			// Список букв разрешенных в последовательности
			wstring letters = L"";
		private:
			// Списки доменных зон интернета
			set <wstring> general, national, user;
			// Основные регулярные выражения модуля
			wregex expressEmail, expressDomain, expressIP;
		public:
			/**
			 * getZones Метод извлечения списка пользовательских зон интернета
			 */
			const set <wstring> & getZones() const noexcept;
			/**
			 * parse Метод парсинга URI строки
			 * @param text текст для парсинга
			 * @return     параметры полученные в результате парсинга
			 */
			const data_t parse(const wstring & text) noexcept;
		public:
			/**
			 * clear Метод очистки результатов парсинга
			 */
			void clear() noexcept;
			/**
			 * setZone Метод установки пользовательской зоны
			 * @param zone пользовательская зона
			 */
			void setZone(const wstring & zone) noexcept;
			/**
			 * setLetters Метод добавления букв алфавита
			 * @param letters список букв алфавита
			 */
			void setLetters(const wstring & letters) noexcept;
			/**
			 * setZones Метод установки списка пользовательских зон
			 * @param zones список доменных зон интернета
			 */
			void setZones(const set <wstring> & zones) noexcept;
			/**
			 * NWT Конструктор
			 * @param letters список букв алфавита
			 * @param text    текст для парсинга
			 */
			NWT(const wstring & letters = L"", const wstring & text = L"") noexcept;
	} nwt_t;
};

#endif // __AWH_NWT__
