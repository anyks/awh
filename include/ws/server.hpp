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

#ifndef __AWH_WS_SERVER_HTTP__
#define __AWH_WS_SERVER_HTTP__

/**
 * Наши модули
 */
#include <ws/ws.hpp>
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
		typedef class WS : public ws_t {
			private:
				/**
				 * update Метод обновления входящих данных
				 */
				void update() noexcept;
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
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова для извлечения пароля
				 */
				void setExtractPassCallback(void * ctx, function <string (const string &, void *)> callback) noexcept;
				/**
				 * setAuthCallback Метод добавления функции обработки авторизации
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void setAuthCallback(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept;
			public:
				/**
				 * setAuthType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void setAuthType(const awh::auth_t::type_t type = awh::auth_t::type_t::BASIC, const awh::auth_t::hash_t hash = awh::auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * WS Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 * @param uri объект работы с URI
				 */
				WS(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : ws_t(fmk, log, uri) {
					// Устанавливаем тип HTTP модуля (Сервер)
					this->web.init(web_t::hid_t::SERVER);
					// Устанавливаем тип модуля (Сервер)
					this->httpType = web_t::hid_t::SERVER;
				}
				/**
				 * ~WS Деструктор
				 */
				~WS() noexcept {}
		} wss_t;
	};
};

#endif // __AWH_WS_SERVER_HTTP__
