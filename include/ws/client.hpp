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

#ifndef __AWH_WS_CORE_CLIENT__
#define __AWH_WS_CORE_CLIENT__

/**
 * Наши модули
 */
#include <ws/core.hpp>
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
		 * WS Класс для работы с клиентом WebSocket
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
				 * user Метод установки параметров авторизации
				 * @param user логин пользователя для авторизации на сервере
				 * @param pass пароль пользователя для авторизации на сервере
				 */
				void user(const string & user, const string & pass) noexcept;
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
				 * @param uri объект работы с URI
				 */
				WS(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : ws_core_t(fmk, log, uri) {
					// Устанавливаем тип HTTP модуля (Клиент)
					this->web.init(awh::web_t::hid_t::CLIENT);
					// Устанавливаем тип модуля (Клиент)
					this->httpType = awh::web_t::hid_t::CLIENT;
				}
				/**
				 * ~WS Деструктор
				 */
				~WS() noexcept {}
		} ws_t;
	};
};

#endif // __AWH_WS_CORE_CLIENT__
