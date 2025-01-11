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

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Socks5 Класс клиента для работы с socks5 прокси-сервером
		 */
		typedef class AWHSHARED_EXPORT Socks5 : public awh::socks5_t {
			private:
				// Пароль пользователя
				string _pass;
				// Логин пользователя
				string _login;
			private:
				/**
				 * cmd Метод получения бинарного буфера запроса
				 */
				void cmd() const noexcept;
				/**
				 * auth Метод получения бинарного буфера авторизации на сервере
				 */
				void auth() const noexcept;
				/**
				 * methods Метод получения бинарного буфера опроса методов подключения
				 */
				void methods() const noexcept;
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
				 * user Метод установки параметров авторизации
				 * @param login логин пользователя для авторизации на сервере
				 * @param pass  пароль пользователя для авторизации на сервере
				 */
				void user(const string & login, const string & pass) noexcept;
			public:
				/**
				 * Socks5 Конструктор
				 * @param log объект для работы с логами
				 */
				Socks5(const log_t * log) noexcept : awh::socks5_t(log), _pass{""}, _login{""} {}
				/**
				 * ~Socks5 Деструктор
				 */
				~Socks5() noexcept {}
		} socks5_t;
	};
};

#endif // __AWH_SOCKS5_CLIENT__
