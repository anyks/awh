/**
 * @file: server.hpp
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

#ifndef __AWH_SOCKS5_SERVER__
#define __AWH_SOCKS5_SERVER__

/**
 * Стандартные модули
 */
#include <functional>

/**
 * Наши модули
 */
#include <socks5/core.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Socks5 Класс сервера для работы с socks5 прокси-сервером
		 */
		typedef class AWHSHARED_EXPORT Socks5 : public awh::socks5_t {
			public:
				/**
				 * Server Структура параметров запрашиваемого сервера
				 */
				typedef struct Server {
					int32_t family; // Тип хоста сервера IPv4/IPv6
					uint32_t port;  // Порт сервера
					string host;    // Хост сервера
					/**
					 * Server Конструктор
					 */
					Server() noexcept : family(AF_INET), port(80), host{""} {}
				} serv_t;
			private:
				// Параметры запрашиваемого сервера
				serv_t _server;
			private:
				// Внешняя функция проверки авторизации
				function <bool (const string &, const string &)> _auth;
			public:
				/**
				 * server Метод извлечения параметров запрашиваемого сервера
				 * @return параметры запрашиваемого сервера
				 */
				const serv_t & server() const noexcept;
			public:
				/**
				 * cmd Метод получения бинарного буфера ответа
				 * @param rep код ответа сервера
				 */
				void cmd(const rep_t rep) const noexcept;
				/**
				 * method Метод получения бинарного буфера выбора метода подключения
				 * @param methods методы авторизаций выбранныйе пользователем
				 */
				void method(const vector <uint8_t> & methods) const noexcept;
				/**
				 * auth Метод получения бинарного буфера ответа на авторизацию клиента
				 * @param login    логин пользователя
				 * @param password пароль пользователя
				 */
				void auth(const string & login, const string & password) const noexcept;
			public:
				/**
				 * parse Метод парсинга входящих данных
				 * @param buffer бинарный буфер входящих данных
				 * @param size   размер бинарного буфера входящих данных
				 */
				void parse(const char * buffer = nullptr, const size_t size = 0) noexcept;
			public:
				/**
				 * reset Метод сброса собранных данных
				 */
				void reset() noexcept;
			public:
				/**
				 * authCallback Метод добавления функции обработки авторизации
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void authCallback(function <bool (const string &, const string &)> callback) noexcept;
			public:
				/**
				 * Socks5 Конструктор
				 * @param log объект для работы с логами
				 */
				Socks5(const log_t * log) noexcept : awh::socks5_t(log), _auth(nullptr) {}
				/**
				 * ~Socks5 Деструктор
				 */
				~Socks5() noexcept {}
		} socks5_t;
	};
};

#endif // __AWH_SOCKS5_SERVER__
