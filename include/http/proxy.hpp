/**
 * @file: proxy.hpp
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

#ifndef __AWH_HTTP_PROXY_SERVER__
#define __AWH_HTTP_PROXY_SERVER__

/**
 * Наши модули
 */
#include <http/core.hpp>
#include <auth/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * HttpProxy Класс для работы с REST прокси-сервером
	 */
	typedef class HttpProxy : public http_t {
		private:
			/**
			 * checkAuth Метод проверки авторизации
			 * @return результат проверки авторизации
			 */
			stath_t checkAuth() noexcept;
		public:
			/**
			 * setRealm Метод установки название сервера
			 * @param realm название сервера
			 */
			void setRealm(const string & realm) noexcept;
			/**
			 * setOpaque Метод установки временного ключа сессии сервера
			 * @param opaque временный ключ сессии сервера
			 */
			void setOpaque(const string & opaque) noexcept;
		public:
			/**
			 * setExtractPassCallback Метод добавления функции извлечения пароля
			 * @param callback функция обратного вызова для извлечения пароля
			 */
			void setExtractPassCallback(function <string (const string &)> callback) noexcept;
			/**
			 * setAuthCallback Метод добавления функции обработки авторизации
			 * @param callback функция обратного вызова для обработки авторизации
			 */
			void setAuthCallback(function <bool (const string &, const string &)> callback) noexcept;
		public:
			/**
			 * setAuthType Метод установки типа авторизации
			 * @param type тип авторизации
			 * @param hash алгоритм шифрования для Digest авторизации
			 */
			void setAuthType(const awh::auth_t::type_t type = awh::auth_t::type_t::BASIC, const awh::auth_t::hash_t hash = awh::auth_t::hash_t::MD5) noexcept;
		public:
			/**
			 * HttpProxy Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			HttpProxy(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept;
			/**
			 * ~HttpProxy Деструктор
			 */
			~HttpProxy() noexcept {}
	} httpProxy_t;
};

#endif // __AWH_HTTP_PROXY_SERVER__
