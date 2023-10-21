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
			 * status Метод проверки текущего статуса
			 * @return результат проверки текущего статуса
			 */
			status_t status() noexcept;
		public:
			/**
			 * realm Метод установки название сервера
			 * @param realm название сервера
			 */
			void realm(const string & realm) noexcept;
			/**
			 * opaque Метод установки временного ключа сессии сервера
			 * @param opaque временный ключ сессии сервера
			 */
			void opaque(const string & opaque) noexcept;
		public:
			/**
			 * extractPassCallback Метод добавления функции извлечения пароля
			 * @param callback функция обратного вызова для извлечения пароля
			 */
			void extractPassCallback(function <string (const string &)> callback) noexcept;
			/**
			 * authCallback Метод добавления функции обработки авторизации
			 * @param callback функция обратного вызова для обработки авторизации
			 */
			void authCallback(function <bool (const string &, const string &)> callback) noexcept;
		public:
			/**
			 * authType Метод установки типа авторизации
			 * @param type тип авторизации
			 * @param hash алгоритм шифрования для Digest авторизации
			 */
			void authType(const awh::auth_t::type_t type = awh::auth_t::type_t::BASIC, const awh::auth_t::hash_t hash = awh::auth_t::hash_t::MD5) noexcept;
		public:
			/**
			 * HttpProxy Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			HttpProxy(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~HttpProxy Деструктор
			 */
			~HttpProxy() noexcept {}
	} httpProxy_t;
};

#endif // __AWH_HTTP_PROXY_SERVER__
