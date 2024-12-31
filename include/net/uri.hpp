/**
 * @file: uri.hpp
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

#ifndef __AWH_URI__
#define __AWH_URI__

/**
 * Стандартные модули
 */
#include <map>
#include <string>
#include <vector>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <functional>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
// Если - это Unix
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

/**
 * Наши модули
 */
#include <net/net.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/hash.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * URI Класс dns ресолвера
	 */
	typedef class AWHSHARED_EXPORT URI {
		public:
			/**
			 * Основные флаги приложения
			 */
			enum class flag_t : uint8_t {
				SCHEMA = 0x01, // Флаг схемы запроса
				HOST   = 0x02, // Флаг хоста запроса
				PATH   = 0x03, // Флаг пути запроса
				PARAMS = 0x04, // Флаг параметров запроса
				ANCHOR = 0x05, // Флаг якоря запроса
				PORT   = 0x06, // Флаг порта адреса
				PASS   = 0x07, // Флаг пароля пользователя
				LOGIN  = 0x08  // Флаг логина пользователя
			};
		public:
			/**
			 * Params Структура параметров URI
			 */
			typedef struct Params {
				uint32_t port; // Порт
				string host;  // Хост
				string user;  // Пользователь
				string pass;  // Пароль
				/**
				 * Params Конструктор
				 */
				Params() noexcept :
				 port(0), host{""}, user{""}, pass{""} {}
			} params_t;
			/**
			 * URL Класс URL-адреса
			 */
			typedef class AWHSHARED_EXPORT URL {
				public:
					// Порт сервера
					uint32_t port;
				public:
					// Тип протокола интернета AF_INET или AF_INET6
					int32_t family;
				public:
					string ip;   // IP-адрес сервера
					string host; // Хост сервера
				public:
					string user; // Пользователь
					string pass; // Пароль
				public:
					string domain; // Доменное имя
					string schema; // Протокол передачи данных
					string anchor; // Якорь URL-запроса
				public:
					// Путь URL-запроса
					vector <string> path;
					// Параметры URL-запроса
					vector <std::pair <string, string>> params;
				public:
					// Функция выполняемая при генерации URL адреса
					function <string (const URL *, const URI *)> callback;
				public:
					/**
					 * clear Метод очистки
					 */
					void clear() noexcept;
				public:
					/**
					 * empty Метод проверки на существование данных
					 * @return результат проверки
					 */
					bool empty() const noexcept;
				public:
					/**
					 * Оператор [=] перемещения параметров URL-адреса
					 * @param url объект URL-адреса для получения параметров
					 * @return    параметры URL-адреса
					 */
					URL & operator = (URL && url) noexcept;
					/**
					 * Оператор [=] присванивания параметров URL-адреса
					 * @param url объект URL-адреса для получения параметров
					 * @return    параметры URL-адреса
					 */
					URL & operator = (const URL & url) noexcept;
				public:
					/**
					 * Оператор сравнения
					 * @param url параметры URL-адреса
					 * @return    результат сравнения
					 */
					bool operator == (const URL & url) noexcept;
				public:
					/**
					 * URL Конструктор перемещения
					 * @param url параметры URL-адреса
					 */
					URL(URL && url) noexcept;
					/**
					 * URL Конструктор копирования
					 * @param url параметры URL-адреса
					 */
					URL(const URL & url) noexcept;
				public:
					/**
					 * URL Конструктор
					 */
					URL() noexcept;
					/**
					 * ~URL Деструктор
					 */
					~URL() noexcept {}
			} url_t;
		private:
			// Объект IP-адресов
			net_t _net;
			// Объект хэширования
			hash_t _hash;
		private:
			// Объект регулярного выражения
			regexp_t _regexp;
		private:
			// Регулярное выражение для парсинга URI
			regexp_t::exp_t _uri;
			// Регулярное выражение для парсинга E-Mail
			regexp_t::exp_t _email;
			// Регулярное выражение для парсинга параметров
			regexp_t::exp_t _params;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * parse Метод получения параметров URL-запроса
			 * @param url строка URL-запроса для получения параметров
			 * @return    параметры URL-запроса
			 */
			url_t parse(const string & url) const noexcept;
		public:
			/**
			 * etag Метод генерации ETag хэша текста
			 * @param text текст для перевода в строку
			 * @return     хэш etag
			 */
			string etag(const string & text) const noexcept;
		public:
			/**
			 * encode Метод кодирования строки в URL-адресе
			 * @param text строка текста для кодирования
			 * @return     результат кодирования
			 */
			string encode(const string & text) const noexcept;
			/**
			 * decode Метод декодирования строки в URL-адресе
			 * @param text строка текста для декодирования
			 * @return     результат декодирования
			 */
			string decode(const string & text) const noexcept;
		public:
			/**
			 * url Метод создания строки URL-запросы из параметров
			 * @param url параметры URL-запроса
			 * @return    URL-запрос в виде строки
			 */
			string url(const url_t & url) const noexcept;
			/**
			 * query Метод создания строки запроса из параметров
			 * @param url параметры URL-запроса
			 * @return    URL-запрос в виде строки
			 */
			string query(const url_t & url) const noexcept;
			/**
			 * origin Метод создания заголовка [origin], для HTTP запроса
			 * @param url параметры URL-запроса
			 * @return    заголовок [origin]
			 */
			string origin(const url_t & url) const noexcept;
		public:
			/**
			 * create Метод создания полного адреса
			 * @param dest адрес места назначения
			 * @param src  исходный адрес для объединения
			 */
			void create(url_t & dest, const url_t & src) const noexcept;
			/**
			 * combine Метод комбинации двух адресов
			 * @param dest адрес места назначения
			 * @param src  исходный адрес для объединения
			 */
			void combine(url_t & dest, const url_t & src) const noexcept;
		public:
			/**
			 * append Метод добавления к URL адресу параметров запроса
			 * @param url    параметры URL-запроса
			 * @param params параметры для добавления
			 */
			void append(url_t & url, const string & params) const noexcept;
		public:
			/**
			 * concat Объединение двух адресов путём создания третьего
			 * @param dest адрес назначения
			 * @param src  исходный адрес для объединения
			 * @return     результирующий адрес
			 */
			url_t concat(const url_t & dest, const url_t & src) const noexcept;
		public:
			/**
			 * split Метод сплита URI на составные части
			 * @param uri строка URI для сплита
			 * @return    список полученных частей URI
			 */
			std::map <flag_t, string> split(const string & uri) const noexcept;
			/**
			 * splitParams Метод выполнения сплита параметров URI
			 * @param uri строка URI для сплита
			 * @return    параметры полученные при сплите
			 */
			vector <std::pair <string, string>> splitParams(const string & uri) const noexcept;
			/**
			 * splitPath Метод выполнения сплита пути
			 * @param path  путь для выполнения сплита
			 * @param delim сепаратор-разделитель для сплита
			 * @return      список параметров пути
			 */
			vector <string> splitPath(const string & path, const char delim = '/') const noexcept;
		public:
			/**
			 * joinParams Метод сборки параметров URI
			 * @param uri параметры URI для сборки
			 * @return    строка полученная при сборке параметров URI
			 */
			string joinParams(const vector <std::pair <string, string>> & uri) const noexcept;
			/**
			 * joinPath Метод сборки пути запроса
			 * @param path  список параметров пути запроса
			 * @param delim сепаратор-разделитель для сплита
			 * @return      строка собранного пути
			 */
			string joinPath(const vector <string> & path, const char delim = '/') const noexcept;
		public:
			/**
			 * params Метод получения параметров URI
			 * @param uri    URI для получения параметров
			 * @param schema протокол передачи данных
			 * @return       параметры полученные из URI
			 */
			params_t params(const string & uri, const string & schema = "") const noexcept;
		public:
			/**
			 * Оператор [=] получения параметров URL-запроса
			 * @param url строка URL-запроса для получения параметров
			 * @return    параметры URL-запроса
			 */
			url_t operator = (const string & url) const noexcept;
			/**
			 * Оператор [=] создания строки URL-запросы из параметров
			 * @param url параметры URL-запроса
			 * @return    URL-запрос в виде строки
			 */
			string operator = (const url_t & url) const noexcept;
		public:
			/**
			 * URI Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			URI(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~URI Деструктор
			 */
			~URI() noexcept {}
	} uri_t;
	/**
	 * Оператор [<<] вывода в поток IP адреса
	 * @param os  поток куда нужно вывести данные
	 * @param url параметры URL-запроса
	 */
	AWHSHARED_EXPORT ostream & operator << (ostream & os, const uri_t::url_t & url) noexcept;
};

#endif // __AWH_URI__
