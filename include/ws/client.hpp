/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WS_CLIENT_HTTP__
#define __AWH_WS_CLIENT_HTTP__

/**
 * Наши модули
 */
#include <ws/ws.hpp>
#include <auth/client.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * WSClient Класс для работы с клиентом WebSocket
	 */
	typedef class WSClient : public ws_t {
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
			 * setUser Метод установки параметров авторизации
			 * @param user логин пользователя для авторизации на сервере
			 * @param pass пароль пользователя для авторизации на сервере
			 */
			void setUser(const string & user, const string & pass) noexcept;
			/**
			 * setAuthType Метод установки типа авторизации
			 * @param type тип авторизации
			 * @param aes  алгоритм шифрования для Digest авторизации
			 */
			void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::aes_t aes = auth_t::aes_t::MD5) noexcept;
		public:
			/**
			 * WSClient Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			WSClient(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : ws_t(fmk, log, uri) {
				// Устанавливаем тип HTTP модуля
				this->web.init(web_t::hid_t::CLIENT);
			}
			/**
			 * ~WSClient Деструктор
			 */
			~WSClient() noexcept {}
	} wsc_t;
};

#endif // __AWH_WS_CLIENT_HTTP__
