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
 * Стандартная библиотека
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
#include <sys/fmk.hpp>
#include <net/net.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * URI Класс dns ресолвера
	 */
	typedef class URI {
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
				u_int port;  // Порт
				string host; // Хост
				string user; // Пользователь
				string pass; // Пароль
				/**
				 * Params Конструктор
				 */
				Params() noexcept : port(0), host(""), user(""), pass("") {}
			} params_t;
			/**
			 * URL Структура URL адреса
			 */
			typedef class URL {
				public:
					// Порт сервера
					u_int port;
				public:
					// Тип протокола интернета AF_INET или AF_INET6
					int family;
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
					vector <pair <string, string>> params;
				public:
					// Функция выполняемая при генерации URL адреса
					function <string (const URL *, const URI *)> fn;
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
					 * URL Конструктор
					 */
					URL() noexcept :
					 port(0), family(AF_INET), ip(""), host(""),
					 user(""), pass(""), domain(""), schema(""),
					 anchor(""), fn(nullptr) {}
			} url_t;
		private:
			// Объект работы с IP-адресами
			net_t _net;
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
			// Создаём объект фреймворка
			const fmk_t * _fmk;
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
			/**
			 * encode Метод кодирования строки в URL-адресе
			 * @param str строка для кодирования
			 * @return    результат кодирования
			 */
			string encode(const string & str) const noexcept;
			/**
			 * decode Метод декодирования строки в URL-адресе
			 * @param str строка для декодирования
			 * @return    результат декодирования
			 */
			string decode(const string & str) const noexcept;
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
			 * append Метод добавления к URL адресу параметров запроса
			 * @param url    параметры URL-запроса
			 * @param params параметры для добавления
			 */
			void append(url_t & url, const string & params) const noexcept;
		public:
			/**
			 * split Метод сплита URI на составные части
			 * @param uri строка URI для сплита
			 * @return    список полученных частей URI
			 */
			map <flag_t, string> split(const string & uri) const noexcept;
			/**
			 * splitParams Метод выполнения сплита параметров URI
			 * @param uri строка URI для сплита
			 * @return    параметры полученные при сплите
			 */
			vector <pair <string, string>> splitParams(const string & uri) const noexcept;
			/**
			 * splitPath Метод выполнения сплита пути
			 * @param path  путь для выполнения сплита
			 * @param delim сепаратор-разделитель для сплита
			 * @return      список параметров пути
			 */
			vector <string> splitPath(const string & path, const string & delim = "/") const noexcept;
		public:
			/**
			 * joinParams Метод сборки параметров URI
			 * @param uri параметры URI для сборки
			 * @return    строка полученная при сборке параметров URI
			 */
			string joinParams(const vector <pair <string, string>> & uri) const noexcept;
			/**
			 * joinPath Метод сборки пути запроса
			 * @param path  список параметров пути запроса
			 * @param delim сепаратор-разделитель для сплита
			 * @return      строка собранного пути
			 */
			string joinPath(const vector <string> & path, const string & delim = "/") const noexcept;
		public:
			/**
			 * params Метод получения параметров URI
			 * @param uri    для получения параметров
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
			 */
			URI(const fmk_t * fmk) noexcept;
			/**
			 * ~URI Деструктор
			 */
			~URI() noexcept;
	} uri_t;
	/**
	 * Оператор [<<] вывода в поток IP адреса
	 * @param os  поток куда нужно вывести данные
	 * @param url параметры URL-запроса
	 */
	ostream & operator << (ostream & os, const uri_t::url_t & url) noexcept;
};

#endif // __AWH_URI__
