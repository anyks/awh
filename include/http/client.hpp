/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
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

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * HttpClient Класс для работы с REST клиентом
	 */
	typedef class HttpClient : public http_t {
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
			void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
		public:
			/**
			 * HttpClient Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			HttpClient(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : http_t(fmk, log, uri) {
				// Устанавливаем тип HTTP модуля
				this->web.init(web_t::hid_t::CLIENT);
			}
			/**
			 * ~HttpClient Деструктор
			 */
			~HttpClient() noexcept {}
	} httpCli_t;
};

#endif // __AWH_HTTP_CLIENT__
