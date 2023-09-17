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

#ifndef __AWH_WS_CORE_SERVER__
#define __AWH_WS_CORE_SERVER__

/**
 * Наши модули
 */
#include <ws/core.hpp>
#include <auth/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * WS Класс для работы с сервером WebSocket
		 */
		typedef class WS : public ws_core_t {
			private:
				/**
				 * commit Метод применения полученных результатов
				 */
				void commit() noexcept;
			public:
				/**
				 * checkKey Метод проверки ключа сервера
				 * @return результат проверки
				 */
				bool checkKey() noexcept;
				/**
				 * checkVer Метод проверки на версию протокола
				 * @return результат проверки соответствия
				 */
				bool checkVer() noexcept;
				/**
				 * checkAuth Метод проверки авторизации
				 * @return результат проверки авторизации
				 */
				stath_t checkAuth() noexcept;
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
				 * WS Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				WS(const fmk_t * fmk, const log_t * log) noexcept : ws_core_t(fmk, log) {
					// Устанавливаем тип HTTP модуля (Сервер)
					this->web.init(awh::web_t::hid_t::SERVER);
					// Устанавливаем тип модуля (Сервер)
					this->httpType = awh::web_t::hid_t::SERVER;
				}
				/**
				 * ~WS Деструктор
				 */
				~WS() noexcept {}
		} ws_t;
	};
};

#endif // __AWH_WS_CORE_SERVER__
