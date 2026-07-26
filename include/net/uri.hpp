/**
 * @file: uri.hpp
 * @date: 2026-03-28
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_NET_URI__
#define __AWH_NET_URI__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <iostream>
#include <functional>
#include <unordered_map>

/**
 * Подключаем заголовочный файл проекта
 */
#include "addr.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс для работы с универсальными идентификаторами ресурсов
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Uniform_Resource_Identifier {
		public:
			/**
			 * @brief Режим формата URI
			 *
			 */
			enum class format_t : uint8_t {
				NONE  = 0x00, // Режим формата URI не определён
				FULL  = 0x01, // Полный формат URI (с указанием схемы и порта)
				SMART = 0x02  // Умный формат URI (с указанием схемы и порта только при их наличии)
			};
			/**
			 * @brief Режим формата URI для печати
			 *
			 */
			enum class item_t : uint8_t {
				NONE     = 0x00, // Режим формата URI для печати не определён
				URI      = 0x01, // Режим формата URI для печати полного URI
				SCHEME   = 0x02, // Режим формата URI для печати схемы URI
				USER     = 0x03, // Режим формата URI для печати параметров пользователя URI
				HOST     = 0x04, // Режим формата URI для печати хоста URI
				PATH     = 0x05, // Режим формата URI для печати пути URI
				QUERY    = 0x06, // Режим формата URI для печати параметров URI
				FRAGMENT = 0x07, // Режим формата URI для печати якоря URI
				ORIGIN   = 0x08, // Режим формата URI для печати происхождения URI
				REQUEST  = 0x09  // Режим формата URI для печати запроса URI
			};
			/**
			 * @brief Тип URI
			 *
			 */
			enum class type_t : uint8_t {
				NONE       = 0x00, // Тип URI не определён
				WS         = 0x01, // URI для протокола WebSocket
				WSS        = 0x02, // URI для протокола WebSocket Secure
				FTP        = 0x03, // URI для протокола FTP
				SSH        = 0x04, // URI для протокола SSH
				UDS 	   = 0x05, // URI для Unix Domain Socket
				FILE 	   = 0x06, // URI для файловой системы
				MQTT       = 0x07, // URI для протокола MQTT
				HTTP       = 0x08, // URI для протокола HTTP
				HTTPS      = 0x09, // URI для протокола HTTPS
				REDIS      = 0x0A, // URI для протокола Redis
				MYSQL 	   = 0x0B, // URI для протокола MySQL
				EMAIL      = 0x0C, // URI для электронной почты
				SOCKS5     = 0x0D, // URI для протокола Socks5
				SCHEME     = 0x0E, // URI для схемы
				POSTGRESQL = 0x0F  // URI для протокола PostgreSQL
			};
		private:
			/**
			 * @brief Структура пользователя URI
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ User {
				// Имя пользователя
				string username;
				// Пароль пользователя
				string password;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit User() noexcept;
			} user_t;
		private:
			// Тип URI
			type_t _type;
		private:
			// Параметры пользователя URI
			user_t _user;
		private:
			// Схема URI
			string _scheme;
			// Якорь URI
			string _fragment;
		private:
			// Объект работы с сетевыми адресами
			unique_ptr <net_addr_t> _addr;
			// Хост URI
			unique_ptr <net::attr_t> _attr;
		private:
			// Путь URI
			vector <string> _path;
			// Параметры URI
			unordered_map <string, string> _query;
		private:
			/**
			 * @brief Функция обратного вызова для генерации параметра URI (например, для генерации контрольной суммы)
			 *
			 * @details Функция обратного вызова вызывается при генерации URI и получает указатель на объект URI.
			 *          Функция должна вернуть строку, которая будет добавлена в конец URI.
			 *
			 * @param uri указатель на объект URI
			 * @return    строка, которая будет добавлена в конец URI
			 */
			function <string (const Uniform_Resource_Identifier *)> _callback;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект для работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Метод определения стандартного порта для текущего типа URI
			 *
			 * @return стандартный порт или 0, если для типа URI он не определён
			 */
			uint16_t defaultPort() const noexcept;
		private:
			/**
			 * @brief Метод добавления схемы URI в результат с разделителем, зависящим от типа URI
			 *
			 * @param result результат, в который добавляется схема URI
			 */
			void appendScheme(string & result) const noexcept;
			/**
			 * @brief Метод добавления параметров пользователя (логин и пароль) в результат
			 *
			 * @param result    результат, в который добавляются параметры пользователя
			 * @param delimiter флаг добавления разделителя "@" после параметров пользователя
			 */
			void appendUser(string & result, const bool delimiter) const noexcept;
		private:
			/**
			 * @brief Метод добавления хоста (и порта для сетевых адресов) в результат
			 *
			 * @param result результат, в который добавляется хост
			 * @param format режим формата URI для генерации
			 */
			void appendHost(string & result, const format_t format) const noexcept;
			/**
			 * @brief Метод добавления порта хоста в результат с учётом формата генерации
			 *
			 * @param result результат, в который добавляется порт
			 * @param port   порт хоста, заданный явно (0 — если не задан)
			 * @param format режим формата URI для генерации
			 */
			void appendPort(string & result, const uint16_t port, const format_t format) const noexcept;
		public:
			/**
			 * @brief Метод очистки URI
			 *
			 */
			void clear() noexcept;
		public:
			/**
			 * @brief Метод проверки на существование данных
			 *
			 * @return результат проверки
			 */
			bool empty() const noexcept;
		public:
			/**
			 * @brief Метод получения типа URI
			 *
			 * @return тип URI
			 */
			type_t type() const noexcept;
		public:
			/**
			 * @brief Метод получения схемы URI
			 *
			 * @return схема URI
			 */
			const string & scheme() const noexcept;
			/**
			 * @brief Метод установки схемы URI
			 *
			 * @param scheme схема URI для установки
			 */
			void scheme(string_view scheme) noexcept;
		public:
			/**
			 * @brief Метод получения параметров пользователя URI
			 *
			 * @return параметры пользователя URI
			 */
			const user_t & user() const noexcept;
			/**
			 * @brief Метод установки параметров пользователя URI
			 *
			 * @param user параметры пользователя URI для установки
			 */
			void user(const user_t & user) noexcept;
			/**
			 * @brief Метод установки логина и пароля пользователя URI
			 *
			 * @param username логин пользователя URI для установки
			 * @param password пароль пользователя URI для установки
			 */
			void user(string_view username, string_view password) noexcept;
		public:
			/**
			 * @brief Метод получения якоря URI
			 *
			 * @return якорь URI
			 */
			const string & fragment() const noexcept;
			/**
			 * @brief Метод установки якоря URI
			 *
			 * @param fragment якорь URI для установки
			 */
			void fragment(string_view fragment) noexcept;
		public:
			/**
			 * @brief Метод получения атрибутов URI
			 *
			 * @return атрибуты URI
			 */
			const net::attr_t * attr() const noexcept;
			/**
			 * @brief Метод установки атрибутов URI
			 *
			 * @param attr атрибуты URI для установки
			 */
			void attr(const net::attr_t * attr) noexcept;
		public:
			/**
			 * @brief Метод получения хоста URI
			 *
			 * @return хост URI
			 */
			string host() const noexcept;
			/**
			 * @brief Метод установки хоста URI
			 *
			 * @param host хост URI для установки
			 */
			void host(string_view host) noexcept;
		public:
			/**
			 * @brief Метод получения порта URI
			 *
			 * @return порт URI
			 */
			uint16_t port() const noexcept;
			/**
			 * @brief Метод установки порта URI
			 *
			 * @param port порт URI для установки
			 */
			void port(const uint16_t port) noexcept;
		public:
			/**
			 * @brief Метод получения пути URI
			 *
			 * @return путь URI
			 */
			const vector <string> & path() const noexcept;
			/**
			 * @brief Метод установки пути URI
			 *
			 * @param path путь URI для установки
			 */
			void path(const vector <string> & path) noexcept;
		public:
			/**
			 * @brief Метод получения параметров URI
			 *
			 * @return параметры URI
			 */
			const unordered_map <string, string> & query() const noexcept;
			/**
			 * @brief Метод установки параметров URI
			 *
			 * @param query параметры URI для установки
			 */
			void query(const unordered_map <string, string> & query) noexcept;
		public:
			/**
			 * @brief Метод парсинга URI-запроса
			 *
			 * @param uri строка URI-запроса для получения параметров
			 * @return    тип URI
			 */
			type_t parse(string_view uri) noexcept;
		public:
			/**
			 * @brief Метод генерации ETag хэша текста
			 *
			 * @param text текст для перевода в строку
			 * @param size размер хэша ETag для генерации (по умолчанию 16 байт)
			 * @return     хэш etag
			 */
			string etag(string_view text, const uint8_t size = 16) const noexcept;
		public:
			/**
			 * @brief Метод генерации строки URI
			 *
			 * @param item   режим элемента URI для генерации
			 * @param format режим формата URI для генерации
			 * @return       строка URI
			 */
			string print(const item_t item = item_t::URI, const format_t format = format_t::SMART) const noexcept;
		public:
			/**
			 * @brief Метод установки функции обратного вызова для генерации параметра URI (например, для генерации контрольной суммы)
			 *
			 * @param cb функция обратного вызова для генерации параметра URI
			 */
			void callback(function <string (const Uniform_Resource_Identifier *)> cb) noexcept;
		public:
			/**
			 * @brief Оператор проверки на существование данных
			 *
			 * @return результат проверки
			 */
			operator bool() const noexcept;
			/**
			 * @brief Оператор получения типа URI
			 *
			 * @return тип URI
			 */
			operator type_t() const noexcept;
			/**
			 * @brief Оператор генерации строки URI
			 *
			 * @return строка URI
			 */
			operator string() const noexcept;
			/**
			 * @brief Оператор получения параметров пользователя URI
			 *
			 * @return параметры пользователя URI
			 */
			operator user_t() const noexcept;
		public:
			/**
			 * @brief Оператор получения атрибутов URI
			 *
			 * @return атрибуты URI
			 */
			operator const net::attr_t * () const noexcept;
			/**
			 * @brief Оператор получения пути URI
			 *
			 * @return путь URI
			 */
			operator const vector <string> & () const noexcept;
			/**
			 * @brief Оператор получения параметров URI
			 *
			 * @return параметры URI
			 */
			operator const unordered_map <string, string> & () const noexcept;
		public:
			/**
			 * @brief Оператор сравнения
			 *
			 * @param uri параметры URI для сравнения
			 * @return    результат сравнения
			 */
			bool operator == (const Uniform_Resource_Identifier & uri) noexcept;
			/**
			 * @brief Оператор неравенства
			 *
			 * @param uri параметры URI для сравнения
			 * @return    результат сравнения
			 */
			bool operator != (const Uniform_Resource_Identifier & uri) noexcept;
		public:
			/**
			 * @brief Оператор парсинга URI-запроса
			 *
			 * @param uri строка URI-запроса для получения параметров
			 * @return    текущий объект
			 */
			Uniform_Resource_Identifier & operator = (string_view uri) noexcept;
			/**
			 * @brief Оператор установки параметров пользователя URI
			 *
			 * @param user параметры пользователя URI для установки
			 * @return     текущий объект
			 */
			Uniform_Resource_Identifier & operator = (const user_t & user) noexcept;
			/**
			 * @brief Оператор установки атрибутов URI
			 *
			 * @param attr атрибуты URI для установки
			 * @return     текущий объект
			 */
			Uniform_Resource_Identifier & operator = (const net::attr_t * attr) noexcept;
			/**
			 * @brief Оператор установки пути URI
			 *
			 * @param path путь URI для установки
			 * @return     текущий объект
			 */
			Uniform_Resource_Identifier & operator = (const vector <string> & path) noexcept;
			/**
			 * @brief Оператор установки параметров URI
			 *
			 * @param query параметры URI для установки
			 * @return      текущий объект
			 */
			Uniform_Resource_Identifier & operator = (const unordered_map <string, string> & query) noexcept;
		public:
			/**
			 * @brief Оператор перемещающего присваивания параметров URI
			 *
			 * @param uri объект URI для получения параметров
			 * @return    параметры URI
			 */
			Uniform_Resource_Identifier & operator = (Uniform_Resource_Identifier && uri) noexcept;
			/**
			 * @brief Оператор присваивания присваивания параметров URI
			 *
			 * @param uri объект URI для получения параметров
			 * @return    параметры URI
			 */
			Uniform_Resource_Identifier & operator = (const Uniform_Resource_Identifier & uri) noexcept;
		public:
			/**
			 * @brief Конструктор перемещения
			 *
			 * @param uri параметры URI для перемещения
			 */
			explicit Uniform_Resource_Identifier(Uniform_Resource_Identifier && uri) noexcept;
			/**
			 * @brief Конструктор копирования
			 *
			 * @param uri параметры URI для копирования
			 */
			explicit Uniform_Resource_Identifier(const Uniform_Resource_Identifier & uri) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Uniform_Resource_Identifier(const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			~Uniform_Resource_Identifier() noexcept;
	} uri_t;
	/**
	 * @brief Оператор [>>] чтения из потока URI
	 *
	 * @param is  поток для чтения
	 * @param uri URI для присвоения
	 */
	__AWH_SHARED_EXPORT__ istream & operator >> (istream & is, uri_t & uri) noexcept;
	/**
	 * @brief Оператор [<<] вывода в поток URI
	 *
	 * @param os  поток куда нужно вывести данные
	 * @param uri URI для присвоения
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const uri_t & uri) noexcept;
};

#endif // __AWH_NET_URI__
