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
#include <regex>
#include <string>
#include <vector>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdlib.h>

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
#include <net/nwk.hpp>

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
					string ip;   // IP адрес сервера
					string host; // Хост сервера
				public:
					string user; // Пользователь
					string pass; // Пароль
				public:
					string domain; // Доменное имя
					string schema; // Протокол передачи данных
					string anchor; // Якорь URL запроса
				public:
					// Путь URL запроса
					vector <string> path;
					// Параметры URL запроса
					vector <pair <string, string>> params;
				public:
					// Функция генерации цифровой подписи запроса
					function <string (const URL *, const URI *)> sign;
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
					 anchor(""), sign(nullptr) {}
			} url_t;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаем объект сети
			const network_t * _nwk;
		public:
			/**
			 * parse Метод получения параметров URL запроса
			 * @param url строка URL запроса для получения параметров
			 * @return    параметры URL запроса
			 */
			const url_t parse(const string & url) const noexcept;
		public:
			/**
			 * etag Метод генерации ETag хэша текста
			 * @param text текст для перевода в строку
			 * @return     хэш etag
			 */
			const string etag(const string & text) const noexcept;
			/**
			 * encode Метод кодирования строки в url адресе
			 * @param str строка для кодирования
			 * @return    результат кодирования
			 */
			const string encode(const string & str) const noexcept;
			/**
			 * decode Метод декодирования строки в url адресе
			 * @param str строка для декодирования
			 * @return    результат декодирования
			 */
			const string decode(const string & str) const noexcept;
		public:
			/**
			 * url Метод создания строки URL запросы из параметров
			 * @param url параметры URL запроса
			 * @return    URL запрос в виде строки
			 */
			const string url(const url_t & url) const noexcept;
			/**
			 * query Метод создания строки запроса из параметров
			 * @param url параметры URL запроса
			 * @return    URL запрос в виде строки
			 */
			const string query(const url_t & url) const noexcept;
			/**
			 * origin Метод создания заголовка [origin], для HTTP запроса
			 * @param url параметры URL запроса
			 * @return    заголовок [origin]
			 */
			const string origin(const url_t & url) const noexcept;
		public:
			/**
			 * split Метод сплита URI на составные части
			 * @param uri строка URI для сплита
			 * @return    список полученных частей URI
			 */
			const vector <string> split(const string & uri) const noexcept;
			/**
			 * splitParams Метод выполнения сплита параметров URI
			 * @param uri строка URI для сплита
			 * @return    параметры полученные при сплите
			 */
			const vector <pair <string, string>> splitParams(const string & uri) const noexcept;
			/**
			 * splitPath Метод выполнения сплита пути
			 * @param path  путь для выполнения сплита
			 * @param delim сепаратор-разделитель для сплита
			 * @return      список параметров пути
			 */
			const vector <string> splitPath(const string & path, const string & delim = "/") const noexcept;
		public:
			/**
			 * joinParams Метод сборки параметров URI
			 * @param uri параметры URI для сборки
			 * @return    строка полученная при сборке параметров URI
			 */
			const string joinParams(const vector <pair <string, string>> & uri) const noexcept;
			/**
			 * joinPath Метод сборки пути запроса
			 * @param path  список параметров пути запроса
			 * @param delim сепаратор-разделитель для сплита
			 * @return      строка собранного пути
			 */
			const string joinPath(const vector <string> & path, const string & delim = "/") const noexcept;
		public:
			/**
			 * params Метод получения параметров URI
			 * @param uri    для получения параметров
			 * @param schema протокол передачи данных
			 * @return       параметры полученные из URI
			 */
			const params_t params(const string & uri, const string & schema = "") const noexcept;
		public:
			/**
			 * URI Конструктор
			 * @param fmk объект фреймворка
			 * @param nwk объект методов для работы с сетью
			 */
			URI(const fmk_t * fmk, const network_t * nwk) noexcept : _fmk(fmk), _nwk(nwk) {}
			/**
			 * ~URI Деструктор
			 */
			~URI() noexcept {}
	} uri_t;
};

#endif // __AWH_URI__
