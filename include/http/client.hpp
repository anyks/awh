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

#ifndef __AWH_HTTP_CLIENT__
#define __AWH_HTTP_CLIENT__

/**
 * Наши модули
 */
#include <http/core.hpp>
#include <auth/client.hpp>

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
		 * Http Класс для работы с REST клиентом
		 */
		typedef class Http : public awh::http_t {
			private:
				/**
				 * checkAuth Метод проверки авторизации
				 * @return результат проверки авторизации
				 */
				stath_t checkAuth() noexcept;
			public:
				/**
				 * setUser Метод установки параметров авторизации
				 * @param user логин пользователя для авторизации на сервере
				 * @param pass пароль пользователя для авторизации на сервере
				 */
				void setUser(const string & user, const string & pass) noexcept;
				/**
				 * setAuthType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void setAuthType(const awh::auth_t::type_t type = awh::auth_t::type_t::BASIC, const awh::auth_t::hash_t hash = awh::auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * Http Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 * @param uri объект работы с URI
				 */
				Http(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept;
				/**
				 * ~Http Деструктор
				 */
				~Http() noexcept {}
		} http_t;
	};
};

#endif // __AWH_HTTP_CLIENT__
