/**
 * @file: client.hpp
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

#ifndef __AWH_SOCKS5_CLIENT__
#define __AWH_SOCKS5_CLIENT__

/**
 * Наши модули
 */
#include <socks5/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Socks5 Класс клиента для работы с socks5 прокси-сервером
		 */
		typedef class Socks5 : public awh::socks5_t {
			private:
				// Логин пользователя
				string login = "";
				// Пароль пользователя
				string password = "";
			private:
				/**
				 * reqCmd Метод получения бинарного буфера запроса
				 */
				void reqCmd() const noexcept;
				/**
				 * reqAuth Метод получения бинарного буфера авторизации на сервере
				 */
				void reqAuth() const noexcept;
				/**
				 * reqMethods Метод получения бинарного буфера опроса методов подключения
				 */
				void reqMethods() const noexcept;
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
				 * clearUser Метод очистки списка пользователей
				 */
				void clearUser() noexcept;
				/**
				 * setUser Метод установки параметров авторизации
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				void setUser(const string & login, const string & password) noexcept;
			public:
				/**
				 * Socks5 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 * @param uri объект для работы с URI
				 */
				Socks5(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : awh::socks5_t(fmk, log, uri) {}
				/**
				 * ~Socks5 Деструктор
				 */
				~Socks5() noexcept {}
		} socks5_t;
	};
};

#endif // __AWH_SOCKS5_CLIENT__
